#pragma once

#include <QAbstractListModel>
#include <QFuture>
#include <QFutureWatcher>
#include <QSet>

#include "daemon.h"
#include "media_structs.h"
#include "tag_structs.h"

class MediaTagModel : public QAbstractListModel {

	Q_OBJECT

public:
	MediaTagModel(Daemon* daemon);
	~MediaTagModel();

	int				rowCount(const QModelIndex&) const override;
	QVariant		data(const QModelIndex&, int) const override;
	bool			removeRows(int, int, const QModelIndex &) override;

	void			Reset();
	void			AddMediaTagByName(const QString&);
	void			AddMediaTagById(const unsigned int);
	void			InsertMediaTag(const ModelTag&);
	void			RemoveMediaTag(const unsigned int tag_id);			//remove tag from media tag view
	void			UnlinkMediaTagByRow(const int row);					//call to daemon to remove the link between media and tag

	bool			TagExist(const unsigned int tag_id) {
		return tag_id_set.find(tag_id) != tag_id_set.end();
	}
	void			UpdateTagName(const unsigned int, const QString&);

	int				GetMediaId() {
		return media_id;
	}
	bool			HasMedia() {
		return has_media;
	}

public slots:

	void SetMedia(const ModelMedia&);
	void OnDaemonFetchTagsComplete();

	//catch these signals from daemon just in case the current displayed media need to update
	//void OnNewMediaTag(const unsigned int, const ModelTag&);
	//void OnRemovedMediaTag(const unsigned int, const unsigned int);

signals:

	void MediaSet(const ModelMedia&);
	void MediaReset();

private:

	Daemon*		daemon;
	bool		has_media;			//if media tag model is currently displaying media
	unsigned int	media_id;
	QVector<ModelTag>					model_tag_vec;
	QSet<unsigned int>					tag_id_set;


	QFutureWatcher<void>				daemon_fetch_tags_future_watcher;
	QFuture<QVector<ModelTag>>	daemon_fetch_tags_future;
};

