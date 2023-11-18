#pragma once
#include <QAbstractTableModel>
#include <QFutureWatcher>

#include "tag_structs.h"
#include "logger.h"

class Daemon;

class TagModel : public QAbstractTableModel
{

	Q_OBJECT

public:
	TagModel(Daemon*);
	~TagModel();

	int				rowCount(const QModelIndex&) const override;
	int				columnCount(const QModelIndex&) const override;
	QVariant		data(const QModelIndex&, int) const override;
	QVariant		headerData(int, Qt::Orientation, int) const override;
	Qt::ItemFlags	flags(const QModelIndex&) const override;
	bool			setData(const QModelIndex&, const QVariant&, int) override;
	bool			removeRows(int, int, const QModelIndex&) override;

	void			InsertTag(const ModelTag&);
	void			RemoveTagById(const unsigned int);
	bool			TagExistById(const unsigned int id);
	void			UpdateTagName(const unsigned int, const QString&);
	void			IncTagMediaCount(const unsigned int);
	void			DecTagMediaCount(const unsigned int);

	void			GetAllTags();
	void			GetTagIdByIndex(const QModelIndex&, unsigned int*);

	void			DaemonAddTag(const QString& new_tag_name);
	void			DaemonRemoveTagById(const unsigned int tag_id);
	void			DaemonRemoveTagByIndex(const QModelIndex& index);
	void			DaemonUpdateTagName(const unsigned int tag_id, const QString& new_name);

public slots:

	void OnDaemonFetchComplete();

signals:
	void TagDoubleClickedId(const unsigned int);

private:
	

	Daemon*						daemon;

	QFutureWatcher<void>			daemon_fetch_future_watcher;

	QFuture<QVector<ModelTag>>		daemon_fetch_future;

	//a vector of tags that can be displayed. hole tags are omitted from this vector
	QVector<ModelTag>					model_tag_vec;
	QSet<unsigned int>					tag_id_set;
};

