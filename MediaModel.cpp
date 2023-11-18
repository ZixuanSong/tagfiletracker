#include "MediaModel.h"
#include "QtConcurrent/qtconcurrentrun.h"
#include "QtConcurrent/qtconcurrentmap.h"
#include <QStringBuilder>
#include "daemon.h"
#include <QSize>


//#include <QFileInfo>

#define INIT_THUMB_GEN_QUEUE_SIZE 50	//number of model medias send to thumbnail provider to generate upon receiving media from daemon 

MediaModel::MediaModel(Daemon *d, ThumbnailProvider *tn_provider) :
	daemon(d),
	thumbnail_provider(tn_provider)
{
	//daemon fetch future watcer will signal once the concurrent runner finishes fetching from daemon
	connect(&daemon_fetch_future_watcher, SIGNAL(finished()), this, SLOT(OnDaemonFetchComplete()));
	connect(this, SIGNAL(ImageToIconCompleted(const int, const QIcon&)), this, SLOT(OnImageToIconComplete(const int, const QIcon&)));

	current_display_mode = NONE;
}


MediaModel::~MediaModel()
{
}

//public reimplement

int
MediaModel::rowCount(const QModelIndex& index) const {
	return model_media_vec.size();
}

QVariant		
MediaModel::data(const QModelIndex& index, int role) const {

	const ModelMedia* m_media_p = &model_media_vec[index.row()];

	switch (role) {
	case Qt::DecorationRole:
	{

		//qDebug() << "dec: " << m_media_p->name;
		return thumbnail_provider->GetMediaThumbnail(m_media_p->id, m_media_p->GetAbsPath());
	}
	case Qt::DisplayRole:
		return m_media_p->name;

	}

	return QVariant();
}

bool
MediaModel::removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) {

	beginRemoveRows(parent, row, row);

	model_media_vec.removeAt(row);

	endRemoveRows();

	return true;
}

/*
void 
MediaModel::SetRootDir(const QString& root_dir) {
	daemon_root_dir = root_dir;
}
*/

void
MediaModel::Reset() {
	beginResetModel();
	model_media_vec.clear();
	media_id_set.clear();
	endResetModel();

	current_display_mode = NONE;
	associated_tag_id_set.clear();
}


void
MediaModel::InsertMedia(const ModelMedia& media) {
	beginInsertRows(QModelIndex(), model_media_vec.size(), model_media_vec.size());

	model_media_vec.push_back(media);
	media_id_set.insert(media.id);

	endInsertRows();
}

void
MediaModel::RemoveMeida(const unsigned int media_id) {
	int index = GetMediaIndexById(media_id);

	removeRow(index);

	media_id_set.remove(media_id);
}

void 
MediaModel::UpdateMediaName(const unsigned int media_id, const QString& new_name) {
	int index = GetMediaIndexById(media_id);
	model_media_vec[index].name = new_name;

	//emit to reflect changed file name
	QModelIndex row_index = this->index(index, index, QModelIndex());
	emit dataChanged(row_index, row_index, { Qt::DisplayRole });
}

void 
MediaModel::UpdateMediaSubdir(const unsigned int media_id, const QString& new_subdir) {
	int index = GetMediaIndexById(media_id);
	model_media_vec[index].sub_path = new_subdir;
	//don't need to emit anything since no display data has been changed
}

void 
MediaModel::GetMediaFullPathByIndex(const QModelIndex& index, QString* out) {
	*out = model_media_vec[index.row()].GetAbsPath();
}

void
MediaModel::GetModelMediaListByIndex(const QModelIndex& start, const QModelIndex& end, QQueue<ModelMedia>* out) {

	if (!start.isValid()) {
		return;
	}


	int end_row;
	if (end.isValid()) {
		end_row = end.row() + 1;
	}
	else {
		end_row = model_media_vec.size();
	}

	for (int i = start.row(); i < end_row; i++) {
		out->enqueue(model_media_vec[i]);
	}

}

void 
MediaModel::GetModelMediaByIndex(const QModelIndex& index, ModelMedia* out) {
	*out = model_media_vec[index.row()];
}

//public slots

void
MediaModel::OnDaemonFetchComplete() {

	//update associated tag
	associated_tag_id_set = std::move(daemon_fetch_future.result().associated_tag_id_set);

	//nothing was fetched
	if (daemon_fetch_future.result().model_media_list.size() == 0) {
		return;
	}

	//inform gui to add these new rows
	beginInsertRows(QModelIndex(), 0, daemon_fetch_future.result().model_media_list.size() - 1);

	model_media_vec = std::move(daemon_fetch_future.result().model_media_list);

	endInsertRows();
	
	//build id to index table
	for (int i = 0; i < model_media_vec.size(); i++) {
		media_id_set.insert(model_media_vec[i].id);
	}

	QQueue<ModelMedia> thumbnail_gen_queue;
	int end_row = INIT_THUMB_GEN_QUEUE_SIZE;
	if (model_media_vec.size() < end_row) {
		end_row = model_media_vec.size();
	}
	
	for (int i = 0; i < end_row; i++) {
		thumbnail_gen_queue.enqueue(model_media_vec[i]);
	}

	thumbnail_provider->UpdateGenerateQueue(thumbnail_gen_queue);
}

void
MediaModel::OnMediaToIconComplete(const unsigned int media_id) {

	//this media no longer exist in this model
	if (media_id_set.find(media_id) == media_id_set.end()) {
		return;
	}

	int index = GetMediaIndexById(media_id);

	QModelIndex row_index = this->index(index, index, QModelIndex());
	emit dataChanged(row_index, row_index, { Qt::DecorationRole });
}

void
MediaModel::UpdateToAllMedia() {
	//reset model
	Reset();

	current_display_mode = ALL;

	//kick start fetch concurrent runner 
	daemon_fetch_future = QtConcurrent::run(daemon, &Daemon::GetAllMedia);
	daemon_fetch_future_watcher.setFuture(daemon_fetch_future);
}

void
MediaModel::UpdateToAllTaglessMedia() {
	Reset();

	current_display_mode = TAGLESS;

	daemon_fetch_future = QtConcurrent::run(daemon, &Daemon::GetAllTaglessMedia);
	daemon_fetch_future_watcher.setFuture(daemon_fetch_future);
}

void
MediaModel::UpdateByTagId(const unsigned int tag_id) {
	Reset();

	current_display_mode = QUERY;

	daemon_fetch_future = QtConcurrent::run(daemon, &Daemon::GetTagMedias, tag_id);
	daemon_fetch_future_watcher.setFuture(daemon_fetch_future);
}

void
MediaModel::UpdateByQuery(const QString& raw_str) {
	Reset();

	current_display_mode = QUERY;

	daemon_fetch_future = QtConcurrent::run(daemon, &Daemon::GetQueryMedia, raw_str);
	daemon_fetch_future_watcher.setFuture(daemon_fetch_future);
}

//private

void
MediaModel::ResolveModelMediaIcon(const unsigned int media_id, const QString& full_path) {

	//QFileInfo file_info(model_media.full_path);
	
	//model_media.icon = icon_provider.icon(file_info);

	//QtConcurrent::run(this, &MediaModel::ResolveImageToIcon, file_info, row);
}

//void 
//MediaModel::ResolveImageToIcon(const QFileInfo& file, const int row) {
	
//}

int
MediaModel::GetMediaIndexById(const unsigned int media_id) {
	
	for (int index = 0; index < model_media_vec.size(); index++) {
		if (model_media_vec[index].id == media_id) {
			return index;
		}
	}
	return -1;
}