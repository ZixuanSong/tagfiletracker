#include "media_list.h"


/*
	MediaList: a class to manipulate media structures maintained in memory
*/

int
MediaList::GetSize() {
	return list_store.size();
}

int
MediaList::InsertMedia(const MediaInfo& new_media) {
	Media *media_ptr;

	Media new_media_buff(new_media);

	list_store.push_back(new_media_buff);
	media_ptr = &(list_store.back());

	id_to_media_table.insert(new_media.id, media_ptr);

	subpathname_to_media_table.insert(new_media.GetSubpathLongName(), media_ptr);

	//add alt name too if it has one
	if (new_media.short_name.size() > 0) {
		subpathaltname_to_media_table.insert(new_media.GetSubpathAltname(), media_ptr);
	}

	return 1;
}

int
MediaList::RemoveMedia(const unsigned int media_id) {
	auto iter = list_store.begin();
	for (; iter != list_store.end(); iter++) {
		if (iter->id == media_id) {
			break;
		}
	}

	//remove from lookup tables first
	id_to_media_table.remove(media_id);
	subpathname_to_media_table.remove(iter->GetSubpathLongName());

	//erase altname from lookup table if it has one
	if (iter->short_name.size() > 0) {
		subpathaltname_to_media_table.remove(iter->GetSubpathAltname());
	}

	list_store.erase(iter);
	return 1;
}

bool
MediaList::MediaExistById(const unsigned int media_id) const {
	return id_to_media_table.find(media_id) != id_to_media_table.end();
}

bool
MediaList::MediaExistBySubpathName(const QString& subpathname) const {

	if (subpathname_to_media_table.find(subpathname) != subpathname_to_media_table.end()) {
		return true;
	}

	if (subpathaltname_to_media_table.find(subpathname) != subpathaltname_to_media_table.end()) {
		return true;
	}

	return false;
}

void
MediaList::GetMediaById(const unsigned int media_id, Media* out) {
	auto iter = id_to_media_table.find(media_id);
	*out = *(iter.value());
}

void
MediaList::GetMediaBySubpathName(const QString& subpathname, Media* out) {
	auto iter = subpathname_to_media_table.find(subpathname);
	if (iter != subpathname_to_media_table.end()) {
		*out = *(iter.value());
        return;
	}

	iter = subpathaltname_to_media_table.find(subpathname);
	*out = *(iter.value());
}

void	
MediaList::GetMediaInfoById(unsigned int media_id, MediaInfo* out) {
	auto iter = id_to_media_table.find(media_id);
	*out = iter.value()->GetMediaInfo();
}

void
MediaList::GetMediaInfoBySubpathName(const QString& subpathname, MediaInfo* out) {

	auto iter = subpathname_to_media_table.find(subpathname);
	if (iter != subpathname_to_media_table.end()) {
		*out = iter.value()->GetMediaInfo();
        return;
	}

	iter = subpathaltname_to_media_table.find(subpathname);
	*out = iter.value()->GetMediaInfo();
}

void
MediaList::GetAllMediaPtr(QVector<Media*> *out) {
	out->reserve(list_store.size());

	for (auto iter = list_store.begin(); iter != list_store.end(); iter++) {
		out->push_back(&(*iter));
	}
}

void
MediaList::UpdateMediaName(const unsigned int media_id, const QString& long_name, const QString& alt_name) {
	Media *media_ptr = *(id_to_media_table.find(media_id));

	QString sub_path_name;
	subpathname_to_media_table.remove(media_ptr->GetSubpathLongName());

	if (media_ptr->short_name.size() > 0) {
		subpathaltname_to_media_table.remove(media_ptr->GetSubpathAltname());
	}

	media_ptr->long_name = long_name;
	media_ptr->short_name = alt_name;

	
	subpathname_to_media_table.insert(media_ptr->GetSubpathLongName(), media_ptr);

	//erase altname from lookup table if it has one
	if (alt_name.size() > 0) {
		
		subpathaltname_to_media_table.insert(media_ptr->GetSubpathAltname(), media_ptr);
	}
}

void
MediaList::UpdateMediaSubdir(const unsigned int media_id, const QString& sub_dir) {
	Media *media_ptr = *(id_to_media_table.find(media_id));
	
	subpathname_to_media_table.remove(media_ptr->GetSubpathLongName());

	if (media_ptr->short_name.size() > 0) {
		subpathaltname_to_media_table.remove(media_ptr->GetSubpathAltname());
	}

	media_ptr->sub_path = sub_dir;
	subpathname_to_media_table.insert(media_ptr->GetSubpathLongName(), media_ptr);

	//erase altname from lookup table if it has one
	if (media_ptr->short_name.size() > 0) {
		
		subpathaltname_to_media_table.insert(media_ptr->GetSubpathAltname(), media_ptr);
	}
}

void 
MediaList::UpdateMediaHash(const unsigned int media_id, const QString& new_hash) {
	Media *media_ptr = *(id_to_media_table.find(media_id));

	media_ptr->hash = new_hash;
}

int
MediaList::GetMediaTagCount(const unsigned int media_id) {
	Media *media_ptr = *(id_to_media_table.find(media_id));
	return media_ptr->tag_id_list.size();
}

int
MediaList::GetMediaTagIds(const unsigned int media_id, QVector<unsigned int> *out) {
	Media *media_ptr = *(id_to_media_table.find(media_id));

	out->reserve(media_ptr->tag_id_list.size());

	for (auto iter = media_ptr->tag_id_list.begin(); iter != media_ptr->tag_id_list.end(); iter++) {
		out->push_back(*iter);
	}

	return 1;
}

bool
MediaList::MediaTagIdExist(const unsigned int media_id, const unsigned int tag_id) {
	Media *media_ptr = *(id_to_media_table.find(media_id));
	return media_ptr->TagIdExist(tag_id);
}

void
MediaList::InsertMediaTag(const unsigned int tag_id, const unsigned int media_id) {
	auto iter = id_to_media_table.find(media_id);
	(*iter)->AddTagId(tag_id);
}

void
MediaList::RemoveMediaTag(const unsigned int tag_id, const unsigned int media_id) {
	auto iter = id_to_media_table.find(media_id);
	(*iter)->RemoveTagId(tag_id);
}

void
MediaList::Dump() {
	printf("Media list:\n");
	for (auto iter = list_store.begin(); iter != list_store.end(); iter++) {
		//printf("[%d]\tsp: %S\t\t%S\t%S\n", iter->id, iter->sub_wpath.data(), iter->alt_name.data(), iter->name.data());
		if (iter->tag_id_list.size() > 0) {
			printf("Tags: [");
			for (auto iter2 = iter->tag_id_list.begin(); iter2 != iter->tag_id_list.end(); iter2++) {
				printf(" %d", *iter2);
			}
			printf("]\n");
		}
	}
}
