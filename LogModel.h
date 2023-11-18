#pragma once

#include <QAbstractListModel>
#include "logger.h"

class LogModel : public QAbstractListModel {

	Q_OBJECT

public:
	LogModel();
	~LogModel();

	QVariant	data(const QModelIndex&, int role) const override;
	int			rowCount(const QModelIndex&) const override;

	void		SetMaxCacheSize(int size);

public slots:

	void	OnNewLogEntry(const LogEntry& entry);						//these two slots can accept const ref because logger obeject will live on same thread
	void	OnNewLogEntries(const QVector<LogEntry>& entries_vec);		//as logmodel object, connection will be direct.

private:

	int						max_cache_size;
	std::deque<LogEntry>	log_cache;
};

