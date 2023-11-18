#include "TagModel.h"
#include "QtConcurrent/qtconcurrentrun.h"
#include "daemon.h"

TagModel::TagModel(Daemon* d) :
	daemon( d )
{
	connect(&daemon_fetch_future_watcher, &QFutureWatcher<void>::finished, this, &TagModel::OnDaemonFetchComplete);
}


TagModel::~TagModel()
{
}

int 
TagModel::rowCount(const QModelIndex& index) const {
	return model_tag_vec.size();
}

int			
TagModel::columnCount(const QModelIndex& index) const {
	return 3;
}

QVariant 
TagModel::data(const QModelIndex& index, int role) const {
	
	const ModelTag* tag_ptr = &model_tag_vec[index.row()];

	switch (role) {
		case Qt::DisplayRole: {
			switch (index.column()) {
			case 0:
				return tag_ptr->name;
			case 1:
				return tag_ptr->media_count;
			case 2:
				return tag_ptr->id;
			}
			break;
		}
		case Qt::EditRole: {
			if (index.column() == 0) {
				return tag_ptr->name;
			}
			break;
		}
	}

	return QVariant();
}

QVariant
TagModel::headerData(int section, Qt::Orientation orientation, int role) const {

	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			switch (section) {
			case 0:
				return QString("Name");

			case 1:
				return QString("Media count");

			case 2:
				return QString("Id");

			default:
				return QVariant();
			}
		}
	}

	return QVariant();
}

Qt::ItemFlags	
TagModel::flags(const QModelIndex &index) const {
	if (index.column() == 0) {
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	}

	return QAbstractItemModel::flags(index);
}

bool			
TagModel::setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) {

	if (role == Qt::EditRole) {

		
		QString new_data = value.toString().trimmed();

		//make sure tag name doesnt include ' or " as it's reserved in query
		for (QChar ch : new_data) {
			if (ch == '\'' || ch == '\"') {
				Logger::Log("Tag name must not contain quotes ( \' or \" )", LogEntry::LT_ERROR);
				return false;
			}
		}

		ModelTag& m_tag_p = model_tag_vec[index.row()];

		//do nothing if new name is the same
		if (m_tag_p.name == new_data) {
			return true;
		}

		//m_tag_p.name = new_data;

		//emit dataChanged(index, index, { role });
	
		DaemonUpdateTagName(m_tag_p.id, new_data);

		return true;
	}

	return false;
}


bool 
TagModel::removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) {
	//only allow 1 row removal at a time
	if (count > 1) {
		return false;
	}

	//save id before deletion
	unsigned int tag_id = model_tag_vec[row].id;

	beginRemoveRows(QModelIndex(), row, row);

	model_tag_vec.erase(model_tag_vec.begin() + row);

	endRemoveRows();

	tag_id_set.remove(tag_id);

	return true;
}

void
TagModel::InsertTag(const ModelTag& new_tag) {
	beginInsertRows(QModelIndex(), model_tag_vec.size(), model_tag_vec.size());

	model_tag_vec.push_back(new_tag);

	endInsertRows();

	tag_id_set.insert(new_tag.id);
}

void
TagModel::RemoveTagById(const unsigned int tag_id) {
	int index = 0;
	for (; index != model_tag_vec.size(); index++) {
		if (model_tag_vec[index].id == tag_id) {
			break;
		}
	}

	removeRow(index);
}

bool
TagModel::TagExistById(const unsigned int id) {
	return tag_id_set.contains(id);
}

void
TagModel::UpdateTagName(const unsigned int tag_id, const QString& new_name) {
	int index = 0;
	for (; index < model_tag_vec.size(); index++) {
		if (model_tag_vec[index].id == tag_id) {
			break;
		}
	}

	model_tag_vec[index].name = new_name;

	emit dataChanged(createIndex(index, 0), createIndex(index, 0));
}

void
TagModel::IncTagMediaCount(const unsigned int tag_id) {
	int index = 0;
	for (; index < model_tag_vec.size(); index++) {
		if (model_tag_vec[index].id == tag_id) {
			break;
		}
	}

	model_tag_vec[index].media_count++;

	emit dataChanged(createIndex(index, 1), createIndex(index, 1));
}

void
TagModel::DecTagMediaCount(const unsigned int tag_id) {
	int index = 0;
	for (; index < model_tag_vec.size(); index++) {
		if (model_tag_vec[index].id == tag_id) {
			break;
		}
	}

	model_tag_vec[index].media_count--;

	emit dataChanged(createIndex(index, 1), createIndex(index, 1));
}

void			
TagModel::GetAllTags() {

	//kick start fetch concurrent runner 
	daemon_fetch_future = QtConcurrent::run(daemon, &Daemon::GetAllTags);
	daemon_fetch_future_watcher.setFuture(daemon_fetch_future);
}

void
TagModel::GetTagIdByIndex(const QModelIndex& index, unsigned int* out) {
	*out = model_tag_vec[index.row()].id;
}

void
TagModel::DaemonAddTag(const QString& new_tag_name) {
	unsigned int unused;
	QtConcurrent::run(daemon, &Daemon::AddTag, new_tag_name, &unused);
}

void
TagModel::DaemonRemoveTagById(const unsigned int tag_id) {
	QtConcurrent::run(this->daemon, &Daemon::RemoveTagById, tag_id);
}

void			
TagModel::DaemonRemoveTagByIndex(const QModelIndex& index) {
	DaemonRemoveTagById(model_tag_vec[index.row()].id);
}

void			
TagModel::DaemonUpdateTagName(const unsigned int tag_id, const QString& new_name) {
	QtConcurrent::run(this->daemon, &Daemon::UpdateTagName, tag_id, new_name);
}


//slots

/*void
TagModel::OnDeleteTag(const unsigned int tag_id) {

	auto iter = model_tag_vec.begin();
	int index = 0;
	for (; iter != model_tag_vec.end(); iter++) {
		if (iter->id == tag_id) {
			break;
		}
		index++;
	}

	beginRemoveRows(QModelIndex(), index, index);

	id_to_index_table.remove(iter->id);
	model_tag_vec.erase(iter);

	endRemoveRows();
}*/

void 
TagModel::OnDaemonFetchComplete() {

	if (daemon_fetch_future.result().size() == 0) {
		return;
	}

	//inform gui to add these new rows
	beginInsertRows(QModelIndex(), 0, daemon_fetch_future.result().size() - 1);

	model_tag_vec = std::move(daemon_fetch_future.result());

	endInsertRows();

	//populate tag_id_set
	for (const ModelTag& tag : model_tag_vec) {
		tag_id_set.insert(tag.id);
	}
}