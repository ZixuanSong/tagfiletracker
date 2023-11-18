#include "MediaTagModel.h"
#include "QtConcurrent/qtconcurrentrun.h"



MediaTagModel::MediaTagModel(Daemon* d) :
	daemon(d)
{
	media_id = 0;
	has_media = false;
	connect(&daemon_fetch_tags_future_watcher, SIGNAL(finished()), this, SLOT(OnDaemonFetchTagsComplete()));
}


MediaTagModel::~MediaTagModel()
{
}

int				
MediaTagModel::rowCount(const QModelIndex& index) const {
	return model_tag_vec.size();
}

QVariant		
MediaTagModel::data(const QModelIndex& index, int role) const {
	if (role == Qt::DisplayRole) {
		return model_tag_vec[index.row()].name;
	}

	return QVariant();
}

bool 
MediaTagModel::removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) {
	
	//not possible to remove multiple tags for now
	if (count > 1) {
		return false;
	}

	unsigned int tag_id = model_tag_vec[row].id;

	beginRemoveRows(parent, row, row);

	tag_id_set.remove(tag_id);
	model_tag_vec.removeAt(row);

	endRemoveRows();

	return true;
}
void			
MediaTagModel::Reset() {
	beginResetModel();
	model_tag_vec.clear();
	endResetModel();

	has_media = false;

	emit MediaReset();
}

void
MediaTagModel::AddMediaTagByName(const QString& tag_name) {
	QtConcurrent::run(daemon, &Daemon::FormLinkByTagName, tag_name, media_id);
}

void
MediaTagModel::AddMediaTagById(const unsigned int tag_id) {
	QtConcurrent::run(daemon, &Daemon::FormLinkByIds, tag_id, media_id);
}

void
MediaTagModel::InsertMediaTag(const ModelTag& new_tag) {
	beginInsertRows(QModelIndex(), model_tag_vec.size(), model_tag_vec.size());

	tag_id_set.insert(new_tag.id);
	model_tag_vec.push_back(new_tag);

	endInsertRows();
}

void
MediaTagModel::RemoveMediaTag(const unsigned int tag_id) {
	int row_id = 0;
	for (; row_id < model_tag_vec.size(); row_id++) {
		if (model_tag_vec[row_id].id == tag_id) {
			break;
		}
	}

	removeRow(row_id);
}

void
MediaTagModel::UnlinkMediaTagByRow(const int row) {
	
	int tag_id = model_tag_vec[row].id;
	removeRow(row);
	QtConcurrent::run(daemon, &Daemon::DestroyLink, tag_id, this->media_id);
}

void
MediaTagModel::UpdateTagName(const unsigned int tag_id, const QString& new_name) {
	int row_id = 0;
	for (; row_id < model_tag_vec.size(); row_id++) {
		if (model_tag_vec[row_id].id == tag_id) {
			break;
		}
	}

	model_tag_vec[row_id].name = new_name;

	emit dataChanged(createIndex(row_id, 0), createIndex(row_id, 0));
}

//slots

void
MediaTagModel::SetMedia(const ModelMedia& m_media) {
	
	//no work is done
	if (m_media.id == media_id) {
		return;
	}

	//start fetching from daemon
	daemon_fetch_tags_future = QtConcurrent::run(daemon, &Daemon::GetMediaTags, m_media.id);
	daemon_fetch_tags_future_watcher.setFuture(daemon_fetch_tags_future);

	//clear list view
	Reset();

	media_id = m_media.id;
	has_media = true;

	emit MediaSet(m_media);
}

void
MediaTagModel::OnDaemonFetchTagsComplete() {

	//return if no tag
	if (daemon_fetch_tags_future.result().size() == 0) {
		return;
	}

	beginInsertRows(QModelIndex(), 0, daemon_fetch_tags_future.result().size());
	model_tag_vec = daemon_fetch_tags_future.result();

	for (auto iter = model_tag_vec.begin(); iter != model_tag_vec.end(); iter++) {
		tag_id_set.insert(iter->id);
	}

	endInsertRows();
}