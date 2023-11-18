#pragma once

/*
	Structs related to tags
*/

#include <QString>
#include <QVector>
#include <QSet>
#include <QMetaType>

/*
	ModelTag - A representation of the tag in daemon with only information necessary for gui.
	For instance, daemon has a vector of media which belongs to the tag but gui doesn't need that information for displaying the tag
*/

struct ModelTag {
	unsigned int	id;
	QString			name;
	unsigned int	media_count;
};

Q_DECLARE_METATYPE(ModelTag);


/*
	Daemon in memory representation of tag
*/


struct Tag {
	unsigned int			id;						//id of this tag, in sync with index of this tag in vector and rowid in sqlite
	unsigned int			count;					//how many media belongs to this tag
	QString			name;					//tag name
	QSet<unsigned int>		media_id_list;				//list of pointers to media which belongs under this tag

	void AddMediaId(unsigned int media_id) {
		media_id_list.insert(media_id);
	}

	void RemoveMediaId(unsigned int media_id) {
		media_id_list.remove(media_id);
	}

	void FormModelTag(ModelTag* out) {
		out->id = this->id;
		out->name = this->name;
		out->media_count = this->media_id_list.size();
	}
};






