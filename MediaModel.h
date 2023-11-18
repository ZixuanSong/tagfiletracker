#pragma once

#include <QAbstractListModel>
#include <QIcon>
#include <QFutureWatcher>
#include <QImage>

#include "media_structs.h"
#include "thumbnail_provider.h"

class Daemon;

enum FILE_EXT {
	NO_SUPPORT,
	PNG,
	JPG
};

class MediaModel : public QAbstractListModel {

	Q_OBJECT

public:

	enum DISPLAY_MODE {
		NONE,
		ALL,
		TAGLESS,
		QUERY
	};

	MediaModel(Daemon*, ThumbnailProvider*);
	~MediaModel();

	//Qt specific override
	int				rowCount(const QModelIndex&) const override;
	QVariant		data(const QModelIndex&, int) const override;
	bool			removeRows(int, int, const QModelIndex &) override;

	//void SetRootDir(const QString& root_dir);

	void Reset();
	void InsertMedia(const ModelMedia& new_media);
	void RemoveMeida(const unsigned int media_id);
	void UpdateMediaName(const unsigned int media_id, const QString& new_name);
	void UpdateMediaSubdir(const unsigned int media_id, const QString& new_subdir);

	void GetMediaFullPathByIndex(const QModelIndex&, QString*);

	void GetModelMediaByIndex(const QModelIndex&, ModelMedia*);

	DISPLAY_MODE	GetDisplayMode() const { return current_display_mode; }
	bool			IsAssociatedTagId(const unsigned int tag_id) const { return associated_tag_id_set.find(tag_id) != associated_tag_id_set.end(); }
	bool			IsMediaInModel(const unsigned int media_id) const { return media_id_set.find(media_id) != media_id_set.end(); }
	void			ResolveModelMediaIcon(const unsigned int, const QString&);
	void			GetModelMediaListByIndex(const QModelIndex&, const QModelIndex&, QQueue<ModelMedia>*);

public slots:

	void UpdateToAllMedia();
	void UpdateToAllTaglessMedia();
	void UpdateByTagId(const unsigned int);
	void UpdateByQuery(const QString&);

	void OnDaemonFetchComplete();
	void OnMediaToIconComplete(const unsigned int);

signals:

	void ImageToIconCompleted(const int, const QIcon&);

private:

	Daemon*							daemon;	
	//QString							daemon_root_dir;

	ThumbnailProvider*				thumbnail_provider;

	QFutureWatcher<void>			daemon_fetch_future_watcher;

	QFuture<MediaModelResult>		daemon_fetch_future;

	DISPLAY_MODE						current_display_mode;
	QSet<unsigned int>					associated_tag_id_set;		//tag ids involved in the current display

	QVector<ModelMedia>					model_media_vec;
	QSet<unsigned int>					media_id_set;

	QHash<unsigned int, QImage>			media_id_to_image_table;

	int									GetMediaIndexById(const unsigned int);
};

