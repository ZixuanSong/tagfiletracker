#pragma once

#include <QString>
#include <QVector>
#include <QSet>
#include <QStringBuilder>
#include <QMetaType>

/*
	Structs related to media
*/

//a convienience struct media model uses to quickly access information it needs to display media
struct ModelMedia {

	unsigned int	id;
	QString		root_dir;			//seems like a waste of space since alot of media will have the same exact root dir and sub path
	QString		sub_path;			//but since QString implements implicit sharing so we should expect the actual string data be 
									//shared unless changed (copy-on-write)

	QString		name;				//always long name with extension
	QString		hash;

	QString		GetAbsPath() const {
		return root_dir % sub_path % '\\' % name;
	}
};

Q_DECLARE_METATYPE(ModelMedia);

struct MediaInfo {
	unsigned int id;
	QString sub_path;		//sub path without root dir path
							//ex. if file A's absolute_path is C:\dir1\dir2\A.ext and rootdir path is C:\dir1, then relative_path is \dir2
	QString long_name;		//name includes extension (no slash in front of the name unlike path)
							//ex. if file A's absolute path is C:\dir1\A.ext file name is A.ext
	QString short_name;		
	QString	hash;			//SHA2 hash of the media


	MediaInfo() = default;

	MediaInfo(QString sub_path, QString long_name, QString alt_name, QString hash) :
			sub_path(std::move(sub_path)),
			long_name(std::move(long_name)),
			short_name(std::move(alt_name)),
			hash(std::move(hash))
	{
	}

	MediaInfo(unsigned int id, QString sub_path, QString long_name, QString alt_name, QString hash) :
		id(id),
		sub_path(std::move(sub_path)),
		long_name(std::move(long_name)),
		short_name(std::move(alt_name)),
		hash(std::move(hash))
	{
	}

	ModelMedia FormModelMedia(const QString& root_dir) const {
		return ModelMedia{ id, root_dir, sub_path, long_name, hash };
	}

	QString GetSubpathLongName() const {
		return sub_path % '\\' % long_name;
	}

	QString GetSubpathAltname() const {
		return sub_path % '\\' % short_name;
	}
};

struct Media : MediaInfo {

	QSet<unsigned int>	tag_id_list;	//a list of tag ids associated with this media

	Media() = default;

	Media(MediaInfo m_info) :
		MediaInfo(std::move(m_info))
	{
	}

	void AddTagId(const unsigned int tag_id) {
		tag_id_list.insert(tag_id);
	}

	void RemoveTagId(const unsigned int tag_id) {
		tag_id_list.remove(tag_id);
	}

	bool TagIdExist(const unsigned int tag_id) {
		return tag_id_list.find(tag_id) != tag_id_list.end();
	}

	MediaInfo GetMediaInfo() {
		return MediaInfo{id, sub_path, long_name, short_name, hash};
	}
};

//returned by daemon contains result of a request sent by media model
struct MediaModelResult {
	QSet<unsigned int> associated_tag_id_set;
	QVector<ModelMedia> model_media_list;
};

