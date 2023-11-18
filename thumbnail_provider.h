#pragma once

#include <QFileIconProvider>
#include <QSet>
#include <QHash>
#include <QCache>
#include <QImage>
#include <QQueue>
#include <QMutex>
#include <QFuture>
#include <QStack>
#include <QLinkedList>
#include <QMap>

#include "logger.h"
#include "media_structs.h"
#include "config.h"

class ThumbnailProvider : public QObject  {

	Q_OBJECT

public:

	ThumbnailProvider(ThumbnailGenConfig config);
	~ThumbnailProvider();
	
	QImage	GetMediaThumbnail(const unsigned int, const QString&);
	void	RemoveMediaThumbnail(const unsigned int);
	void	UpdateGenerateQueue(const QQueue<ModelMedia>&);

	bool	IsExtensionSupported(const QString&) const;
	bool	IsExtensionSupportedByName(const QString&) const;
	//void	SetSize(const QSize&);

	void	SetConfig(const ThumbnailGenConfig& config);

	//static void ffmpeg_log_callback(void *ptr, int level, const char *fmt, va_list vargs);

signals:
	
	void NewThumbnail(const unsigned int);

private:

	ThumbnailGenConfig				config;
	QMutex							config_lock;
	QSize							icon_size;

	//qint64 total_time;

	static const QSet<QString>		supported_img_ext;
	static const QSet<QString>		supported_vid_ext;

	static QFileIconProvider		file_icon_provider;
	static QMap<QString, QImage>	ext_to_image_map;		//use map because <16 element map lookup is faster than hash 
															//I dont expect user to store more than 16 types of unsupported extension
	
	
	QImage							default_icon;

	QMutex							cache_lock;
	QCache<unsigned int, QImage>	media_id_to_image_cash;

	QMutex							req_queue_lock;
	QQueue<ModelMedia>				req_queue;

	QFuture<void>					thumbnail_gen_future;

	QIcon GetIconByFIPType(QFileIconProvider::IconType);		
	QImage GetDefaultProgramImageByFileInfo(const QFileInfo&);	//performs pixmap operation, call from gui thread
	int GetImageThumbnail(const QString& full_path, QImage** out_image);
	int GetVideoThumbnail(const QString& full_path, QImage** out_image);

	QSize	GetIconSize();

	void	GenerateDefaultIcon();
	void	ThumbnailGenerateMain();
	void	TestAndGenerateThumbnail(const ModelMedia&);

	void	LogFFmpegError(const QString& msg, int err);
};