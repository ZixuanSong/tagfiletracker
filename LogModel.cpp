#include "LogModel.h"
#include <QDialog>
#include <QBrush>


LogModel::LogModel() {
}


LogModel::~LogModel()
{
} 

QVariant 
LogModel::data(const QModelIndex& index, int role) const { 

	const LogEntry *entry = &(log_cache.at(index.row()));

	if (role == Qt::DisplayRole) {
		return entry->content;
	}
	else if (role == Qt::BackgroundRole) {
		QBrush background_brush;
		QColor brush_color;

		switch (entry->type) {
		case LogEntry::LT_ERROR:
			brush_color.setRgb(255, 10, 10, 75);
			break;
		case LogEntry::LT_WARNING:
			brush_color.setRgb(250, 210, 1, 75);
			break;
		case LogEntry::LT_SUCCESS:
			brush_color.setRgb(0, 255, 0, 75);
			break;
		case LogEntry::LT_ATTN:
			brush_color.setRgb(0, 178, 255, 75);
			break;
		case LogEntry::LT_MONITOR:
			brush_color.setRgb(255, 63, 251, 75);
			break;
		case LogEntry::LT_APISERVER:
			brush_color.setRgb(255, 128, 0, 75);
			break;
		default:
			return QVariant();
		}

		background_brush.setStyle(Qt::SolidPattern);
		background_brush.setColor(brush_color);
		return background_brush;
	}

	return QVariant();
}

int LogModel::rowCount(const QModelIndex& parent) const {
	return log_cache.size();
}

void		
LogModel::SetMaxCacheSize (int size) {
	max_cache_size = size;
}


//slots

void
LogModel::OnNewLogEntry(const LogEntry& new_log) {

	int cache_size = log_cache.size();

	//inform gui top row has been removed
	if (cache_size == max_cache_size) {
		beginRemoveRows(QModelIndex(), 0, 0);
		log_cache.pop_front();
		endRemoveRows();

		cache_size--;
	}

	//inform gui a new row has been inserted
	beginInsertRows(QModelIndex(), cache_size, cache_size);
	log_cache.push_back(new_log);
	endInsertRows();
	
}

void	
LogModel::OnNewLogEntries(const QVector<LogEntry>& entries_vec) {

	int cache_size = log_cache.size();
	int vec_size = entries_vec.size();

	if (cache_size + vec_size > max_cache_size) {

		int empty_amt = vec_size - (max_cache_size - cache_size);

		if (empty_amt >= max_cache_size) {
			beginResetModel();
			log_cache.clear();
			endResetModel();
			cache_size = 0;
		}
		else {
			beginRemoveRows(QModelIndex(), 0, empty_amt - 1);
			log_cache.erase(log_cache.cbegin(), log_cache.cbegin() + empty_amt);
			endRemoveRows();

			cache_size = log_cache.size();
		}
	}
	
	//inform gui new rows will be inserted
	beginInsertRows(QModelIndex(), cache_size, (cache_size + vec_size) - 1);
	log_cache.insert(log_cache.cend(), entries_vec.cbegin(), entries_vec.cend());
	endInsertRows();

}