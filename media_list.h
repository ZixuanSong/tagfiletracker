#pragma once

#include <QLinkedList>
#include <QHash>

#include "media_structs.h"

class MediaList {
public:

	int		GetSize();

	int		InsertMedia(const MediaInfo&);
	int		RemoveMedia(const unsigned int);

	bool	MediaExistById(const unsigned int) const;
	bool	MediaExistBySubpathName(const QString&) const;

	void	GetMediaById(unsigned int, Media*);
	void	GetMediaBySubpathName(const QString&, Media*);
	void	GetMediaInfoById(unsigned int, MediaInfo*);
	void	GetMediaInfoBySubpathName(const QString&, MediaInfo*);

	void	GetAllMediaPtr(QVector<Media*>*);

	//TODO: consider rvalue ref overloard
	void	UpdateMediaName(const unsigned int media_id, const QString& long_name, const QString& short_name);
	void	UpdateMediaSubdir(const unsigned int media_id, const QString& sub_dir);
	void	UpdateMediaHash(const unsigned int media_id, const QString& new_hash);

	//media tag related
	int		GetMediaTagCount(const unsigned int);
	int		GetMediaTagIds(const unsigned int, QVector<unsigned int>*);
	bool	MediaTagIdExist(const unsigned int, const unsigned int);
	void	InsertMediaTag(const unsigned int, const unsigned int);
	void	RemoveMediaTag(const unsigned int, const unsigned int);

	void Dump();

private:

	QLinkedList<Media>	list_store;

	QHash<unsigned int, Media*>	id_to_media_table;
	QHash<QString, Media*>	subpathname_to_media_table;
	QHash<QString, Media*>	subpathaltname_to_media_table;

};