#pragma once

#include <QQueue>
#include <QHash>
#include <QVector>

#include "tag_structs.h"
#include "media_structs.h"

/*
	TagList class is the datastructer that holds all tags loaded from
	database in memory. It specifies how tags are inserted and removed.
	TagList does not update the physical database, it should be done
	by the user of this class so that database implementation could be
	flexible, detached from TagList.

	Design decisions:

	1. why go through the trouble of having an elaborate taglist class?
	   Why not just have a list or a vector in daemon class?

	I expect accessing one or more tags to be very frequent due to search
	by tag is going to be the primary operation. we have absolutely have to
	take advantage of cache so vector is the go to implementation accompanied
	by a look up table when searching by name. There will be holes when deleting
	tags. To circumvent this we manage ids ourselves by filling those holes
	manually. An important constraint is that tag id should correspond to vector
	index which holds those tags.

	2. Should each function check if argument is valid?

	No. Functions exposed by core structures are to be used by daemon directly.
	Daemon is expected to know the correct order to call functions and perform
	validation checks for each argument and edge cases.

	3. Does TagList needs to be thread-safe?

	Not currently. The only time two threads could access taglist is during
	daemon initialization and GUI inputs. But that time is so miniscure
	it's almost impossible to do it. This may change however as tag grows
	large and the delay becomes to apperent.
*/

class TagList {
public:
	//TODO: possibly think of a way to reserve tag_list to speed up inserting speed

	bool Empty() const;

	unsigned int GetSize() const;

	//insert tag loaded from database, might create holes (free indexes)
	void InsertSavedTag(const Tag&);

	//insert tag created by user
	void InsertNewTag(const QString&, unsigned int*);

	void RemoveTagById(const unsigned int);
	void RemoveTagByName(const QString&);

	//always copy the content not return a pointer just incase vector resize and old pointer becomes invalid
	//void GetTagBySequentialIndex(const unsigned int, Tag*);
	void GetAllTags(QVector<Tag>*) const;
	void GetTagById(const unsigned int, Tag*) const;
	void GetTagByName(const QString&, Tag*) const;
	void GetTagIdByName(const QString&, unsigned int*) const;

	int InsertTagMedia(const unsigned int, const unsigned int);
	int RemoveTagMedia(const unsigned int, const unsigned int);

	int UpdateTagName(const unsigned int, const QString&);

	inline bool TagExistByName(const QString& name) const {
		return (tag_name_to_id_table.find(name) != tag_name_to_id_table.end());
	}
	inline bool TagExistById(const unsigned int id) const {
		return (id < (unsigned int) tag_vector.size() && !free_index_set.contains(id));
	}


	//for debug
	void DumpTags(bool) const;

private:
	QVector<Tag>									tag_vector;						//stores actual tag
	QHash<QString, unsigned int>		tag_name_to_id_table;			//tag name lookup table
	QQueue<unsigned int>							free_index_queue;				//next free index is top of this queue
	QSet<unsigned int>								free_index_set;		//set of free indexes
};
