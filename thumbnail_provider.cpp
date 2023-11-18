#include "thumbnail_provider.h"
#include "QtConcurrent/qtconcurrentrun.h"
#include "error.h"
#include <QStringBuilder>

extern "C" {

#include "libavcodec/adts_parser.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

}

#include <QDateTime>
#include <qdebug.h>

//define static members
const QSet<QString>		ThumbnailProvider::supported_img_ext{ "jpg", "png", "jpeg","gif" };
const QSet<QString>		ThumbnailProvider::supported_vid_ext{ "webm", "mp4", "mov" };
QFileIconProvider		ThumbnailProvider::file_icon_provider;
QMap<QString, QImage>	ThumbnailProvider::ext_to_image_map;

ThumbnailProvider::ThumbnailProvider(ThumbnailGenConfig config) :
	config(std::move(config)),
	icon_size(QSize(config.icon_size.w, config.icon_size.h))
{
	GenerateDefaultIcon();

	media_id_to_image_cash.setMaxCost(config.cache_size);

	//av_log_set_callback(&ThumbnailProvider::ffmpeg_log_callback);
}


ThumbnailProvider::~ThumbnailProvider()
{
	if (thumbnail_gen_future.isRunning()) {
		thumbnail_gen_future.waitForFinished(); //block until thread exits
	}
}

QImage
ThumbnailProvider::GetMediaThumbnail(const unsigned int media_id, const QString& full_path) {

	//since default program icon is retrieved by file icon provider
	//it falls on GUI thread (this function caller) to perform 
	//default program QIcon to QImage conversion
	QFileInfo file_info(full_path);
	if (!IsExtensionSupported(file_info.suffix())) {

		auto ext_to_image_map_iter = ThumbnailProvider::ext_to_image_map.find(file_info.suffix());
		if (ext_to_image_map_iter != ThumbnailProvider::ext_to_image_map.end()) {
			//extension image is cached
			return *ext_to_image_map_iter;
		}

		QImage default_prog_thumbnail = GetDefaultProgramImageByFileInfo(file_info);
		ThumbnailProvider::ext_to_image_map.insert(file_info.suffix(), default_prog_thumbnail);
		return default_icon;
	}

	//media is supported for thumbnail generation:
	
	//failed to obtain cache lock
	if (!cache_lock.tryLock()) {
		return default_icon;
	}

	//------- cache lock obtained---------

	//cached thumbnail found
	if (media_id_to_image_cash.contains(media_id)) {

		QImage cached_icon = media_id_to_image_cash.object(media_id)->copy();
	
		cache_lock.unlock();
		return cached_icon;
	}

	cache_lock.unlock();

	//this media thumbnail was not cached
	return default_icon;
}

void	
ThumbnailProvider::RemoveMediaThumbnail(const unsigned int media_id) {

	cache_lock.lock();

	if (!media_id_to_image_cash.contains(media_id)) {
		//media icon has not been gen yet, might be in update queue, might be gening right now but not in cache yet
		//TODO: handle that

		cache_lock.unlock();
		return;
	}

	media_id_to_image_cash.remove(media_id);

	cache_lock.unlock();
}

void
ThumbnailProvider::UpdateGenerateQueue(const QQueue<ModelMedia>& new_queue) {

	req_queue_lock.lock();

	req_queue.clear();
	req_queue = std::move(new_queue);

	req_queue_lock.unlock();

	//kick start generating thread if not running already
	if (!thumbnail_gen_future.isRunning()) {
		thumbnail_gen_future = QtConcurrent::run(this, &ThumbnailProvider::ThumbnailGenerateMain);
	}
}

bool
ThumbnailProvider::IsExtensionSupported(const QString& ext) const {

	if (ThumbnailProvider::supported_img_ext.contains(ext) || ThumbnailProvider::supported_vid_ext.contains(ext)) {
		return true;
	}

	return false;
}

bool	
ThumbnailProvider::IsExtensionSupportedByName(const QString& name) const {
	int last_dot_idx = name.lastIndexOf('.');
	if (last_dot_idx < 0) {
		//no extension found automatically assume not supported
		return false;
	}

	return IsExtensionSupported(name.mid(last_dot_idx + 1));
}

/*
void
ThumbnailProvider::SetSize(const QSize& size) {
	icon_size = size;
	GenerateDefaultIcon();
}
*/

//private

QIcon
ThumbnailProvider::GetIconByFIPType(QFileIconProvider::IconType type) {
	return ThumbnailProvider::file_icon_provider.icon(type);
}

QImage
ThumbnailProvider::GetDefaultProgramImageByFileInfo(const QFileInfo& file_info) {
	QIcon icon = ThumbnailProvider::file_icon_provider.icon(file_info);
	QImage image = icon.pixmap(GetIconSize()).toImage();
	if (image.size() == GetIconSize()) {
		return image;
	}
	
	return image.scaled(GetIconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

int
ThumbnailProvider::GetImageThumbnail(const QString &full_path, QImage** out_image) {

	*out_image = new QImage(full_path);

	*(*out_image) = (*out_image)->scaled(GetIconSize() * 4, Qt::KeepAspectRatio).scaled(GetIconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);		//around 26.5 secs
	//return image.scaled(icon_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);																//around 28 secs
	//return image.scaled(icon_size, Qt::KeepAspectRatio);																							//around 23 secs

	return 1;
}

int 
ThumbnailProvider::GetVideoThumbnail(const QString& full_path, QImage** out_image) {

	int ret, stream_idx;
	
	//ffmpeg related objects
	AVFormatContext *context = NULL;
	AVCodecContext *codec_context = NULL;
	AVPacket *packet = NULL;
	AVFrame *frame = NULL;
	SwsContext *sws_context = NULL;

	//final image buffers
	uint8_t *img_data_buff[4] = {NULL, NULL, NULL, NULL};
	uint8_t **img_data_buff_ptr = img_data_buff;
	int img_linesizes[4];

	QImage *new_img;
		
	context = avformat_alloc_context();
	if (context == NULL) {
		Logger::Log(QString(FFMPEG_ALLOC_MSG) % "- AVFormatContext", LogEntry::LT_ERROR);
		ret = -Error::FFMPEG_ALLOC;
		goto end;
	}

	ret = avformat_open_input(&context, full_path.toLocal8Bit(), NULL, NULL);
	if (ret < 0) {
		LogFFmpegError(FFMPEG_OPEN_INPUT_MSG, ret);
		ret = -Error::FFMPEG_OPEN_INPUT;
		goto end;
	}

	

	for (int i = 0; i < context->nb_streams; i++) {
		if (context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			//interested in video stream only

			AVCodec *codec = avcodec_find_decoder(context->streams[i]->codec->codec_id);
			if (codec == NULL) {
				Logger::Log(FFMPEG_FIND_CODEC_MSG, LogEntry::LT_ERROR);
				ret = -Error::FFMPEG_FIND_CODEC;
				goto end;
			}

			codec_context = avcodec_alloc_context3(codec);
			if (codec_context == NULL) {
				Logger::Log(QString(FFMPEG_ALLOC_MSG) % "- AVCodecContext", LogEntry::LT_ERROR);
				ret = -Error::FFMPEG_ALLOC;
				goto end;
			}

			ret = avcodec_parameters_to_context(codec_context, context->streams[i]->codecpar);
			if (ret < 0) {
				LogFFmpegError(FFMPEG_COPY_PARAM_MSG, ret);
				ret = -Error::FFMPEG_COPY_PARAM;
				goto end;
			}

			ret = avcodec_open2(codec_context, codec, NULL);
			if (ret < 0) {
				LogFFmpegError(FFMPEG_OPEN_CODEC_MSG, ret);
				ret = -Error::FFMPEG_OPEN_CODEC;
				goto end;
			}

			stream_idx = i;
			break;
		}
	}

	packet = av_packet_alloc();
	if (packet == NULL) {
		Logger::Log(QString(FFMPEG_ALLOC_MSG) % "- AVPacket", LogEntry::LT_ERROR);
		ret = -Error::FFMPEG_ALLOC;
		goto end;
	}

	frame = av_frame_alloc();
	if (frame == NULL) {
		Logger::Log(QString(FFMPEG_ALLOC_MSG) % "- AVFrame", LogEntry::LT_ERROR);
		ret = -Error::FFMPEG_ALLOC;
		goto end;
	}

	for (;;) {

		for(;;) {
			ret = av_read_frame(context, packet);
			if (ret < 0) {
				LogFFmpegError(FFMPEG_READ_FRAME_MSG, ret);
				ret = -Error::FFMPEG_READ_FRAME;
				goto end;
			}

			if (packet->stream_index != stream_idx) {
				av_packet_unref(packet);
				continue;
			}

			break;
		}

		ret = avcodec_send_packet(codec_context, packet);
		if (ret < 0) {

			if (ret == AVERROR(EAGAIN)) {
				av_packet_unref(packet);
				continue;
			}

			LogFFmpegError(FFMPEG_DECODE_PACKET_MSG, ret);
			ret = -Error::FFMPEG_DECODE_PACKET;
			goto end;
		}

		ret = avcodec_receive_frame(codec_context, frame);
		if (ret < 0) {

			if (ret == AVERROR(EAGAIN)) {
				av_packet_unref(packet);
				av_frame_unref(frame);
				continue;
			}

			LogFFmpegError(FFMPEG_RECV_FRAME_MSG, ret);
			ret = -Error::FFMPEG_RECV_FRAME;
			goto end;
		}

		break;
	}


	sws_context = sws_getContext(frame->width, frame->height, (AVPixelFormat) frame->format, frame->width, frame->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);
	if (sws_context == NULL) {
		Logger::Log(FFMPEG_GET_SWS_CONTEXT_MSG, LogEntry::LT_ERROR);
		ret = -Error::FFMPEG_GET_SWS_CONTEXT;
		goto end;
	}

	ret = av_image_alloc(img_data_buff_ptr, img_linesizes, frame->width, frame->height, AV_PIX_FMT_RGB24, 1);
	if (ret < 0) {
		Logger::Log(QString(FFMPEG_ALLOC_MSG) % "- av_image", LogEntry::LT_ERROR);
		ret = -Error::FFMPEG_ALLOC;
		goto end;
	}

	ret = sws_scale(sws_context, frame->data, frame->linesize, 0, frame->height, img_data_buff_ptr, img_linesizes);
	if (ret < 0) {
		LogFFmpegError(FFMPEG_SWS_SCALE_MSG, ret);
		ret = -Error::FFMPEG_SWS_SCALE;
		goto end;
	}

	
	new_img = new QImage(	img_data_buff_ptr[0], 
							frame->width, 
							frame->height, 
							img_linesizes[0], 
							QImage::Format_RGB888);		//although img data is required to be valid through out the life of QImage, calling scaled() later will
														//create copies of this image so cleanup function is not necessary

	*new_img = new_img->scaled(GetIconSize() * 4, Qt::KeepAspectRatio).scaled(GetIconSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
	*out_image = new_img;

	ret = 1;

end:

	sws_freeContext(sws_context);

	av_packet_free(&packet);
	av_frame_free(&frame);

	avcodec_free_context(&codec_context);
	avformat_close_input(&context);

	av_free(img_data_buff[0]);

	return ret;
}

void	
ThumbnailProvider::SetConfig (const ThumbnailGenConfig& config) {
	config_lock.lock();

	QSize new_icon_size = QSize(config.icon_size.w, config.icon_size.h);
	bool icon_size_changed = icon_size != new_icon_size;
	icon_size = new_icon_size;

	cache_lock.lock();

	media_id_to_image_cash.setMaxCost(config.cache_size);

	if (icon_size_changed) {
		GenerateDefaultIcon();
		media_id_to_image_cash.clear();
	}

	cache_lock.unlock();

	this->config = config;
	config_lock.unlock();
}

QSize	
ThumbnailProvider::GetIconSize () {
	config_lock.lock();

	QSize ret = icon_size;

	config_lock.unlock();

	return ret;
}

void
ThumbnailProvider::GenerateDefaultIcon() {
	QPixmap default_pixmap = GetIconByFIPType(QFileIconProvider::File).pixmap(GetIconSize());
	default_icon = default_pixmap.toImage().scaled(GetIconSize());
}

//main entry point for thumbnail generation thread
void
ThumbnailProvider::ThumbnailGenerateMain() {

	for (;;) {

		req_queue_lock.lock();

		if (req_queue.empty()) {

			req_queue_lock.unlock();
			break;
		}

		ModelMedia buff = req_queue.dequeue();
		req_queue_lock.unlock();

		if (!IsExtensionSupportedByName(buff.name)) {
			continue;
		}

		TestAndGenerateThumbnail(buff);
	}
}

void
ThumbnailProvider::TestAndGenerateThumbnail(const ModelMedia& m_media) {
	cache_lock.lock();

	//this media id thumbnail has been generated already
	if (media_id_to_image_cash.contains(m_media.id)) {

		cache_lock.unlock();
		return;
	}

	//has to unlock first, no way to upgrade to write lock from read
	cache_lock.unlock();

	QFileInfo file_info(m_media.GetAbsPath());
	
	QImage *thumbnail;					//onwership will be passed onto QCache so no need to worry about freeing unless error occurs before passing ownership
	
	int ret;

	if (ThumbnailProvider::supported_vid_ext.contains(file_info.suffix())) {
		//use ffmpeg to generate thumbnail of videos
		ret = GetVideoThumbnail(m_media.GetAbsPath(), &thumbnail);
	}
	else {
		ret = GetImageThumbnail(m_media.GetAbsPath(), &thumbnail);
	}

	if (ret < 0) {
		//something went wrong at generating thumbnail
		delete thumbnail;	//free memory
		Logger::Log(QString("Failed to generate thumbnail for media: ") % m_media.name, LogEntry::LT_WARNING);
		return;
	}

	cache_lock.lock();

	//each image has a cost of 1, the cost is counting how many images not how much memory each image is using
	media_id_to_image_cash.insert(m_media.id, thumbnail, 1);

	cache_lock.unlock();

	emit NewThumbnail(m_media.id);
}

void
ThumbnailProvider::LogFFmpegError(const QString& msg, int err) {

	char err_buff[128];		//length commonly used in ffmpeg source
	av_strerror(err, err_buff, sizeof(err_buff));

	Logger::Log(msg % ": " % QString(err_buff), LogEntry::LT_ERROR);
}

/*
void 
ThumbnailProvider::ffmpeg_log_callback(void *ptr, int level, const char *fmt, va_list vargs) {
	qDebug() << fmt;
}
*/