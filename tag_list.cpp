#include "tag_list.h"

bool
TagList::Empty() const {

	//if the main store vector is empty then tag list is empty
	if (tag_vector.empty()) {
		return true;
	}

	//all element of tag vector is free index it's empty
	if (tag_vector.size() == free_index_queue.size()) {
		return true;
	}

	return false;
}

unsigned int
TagList::GetSize() const {
	//size is the number of tags in tag list minus the number of holes
	return tag_vector.size() - free_index_queue.size();
}

void
TagList::InsertSavedTag(const Tag& tag) {

	//dummy tags used to fill in holes in vector
	Tag dummy;

	//if the loaded tag id is not what the next index in tag_list should be
	//then we have holes, we fill them up with dummy tags and add the index
	//to free index queue
	while (tag_vector.size() != tag.id) {
		free_index_queue.enqueue((unsigned int)tag_vector.size());
		free_index_set.insert((unsigned int)tag_vector.size());
		tag_vector.push_back(dummy);
	}

	//finally next index in tag_list matches tag.id we inser it
	tag_vector.push_back(tag);

	//add entry to tag_table
	tag_name_to_id_table.insert(tag.name, tag.id);
}

void
TagList::InsertNewTag(const QString& name, unsigned int *id) {
	unsigned int next_id;
	Tag tmp;

	//try to use free spot first
	if (!free_index_queue.empty()) {
		next_id = free_index_queue.dequeue();
		free_index_set.remove(next_id);
	}
	else {
		//if there is no free spot then insert a new tag in vector
		next_id = (unsigned int)tag_vector.size();
		tag_vector.push_back(tmp);
	}

	*id = next_id;

	tag_vector[next_id].id = next_id;
	tag_vector[next_id].count = 0;
	tag_vector[next_id].name = name;

	//add entry to tag_table
	tag_name_to_id_table.insert(name, next_id);
}

void
TagList::RemoveTagById(const unsigned int id) {
	//remove tag_table entry
	tag_name_to_id_table.remove(tag_vector[id].name);

	//clear entry from vector
	tag_vector[id].id = -1;

	//this id is now free
	free_index_queue.enqueue(id);
	free_index_set.insert(id);
}

void
TagList::RemoveTagByName(const QString& name) {
	auto iter = tag_name_to_id_table.find(name);
	RemoveTagById(iter.value());
}

void
TagList::GetAllTags(QVector<Tag> *out) const {
	out->reserve(GetSize());
	for (auto iter = tag_vector.begin(); iter != tag_vector.end(); iter++) {
		if (free_index_set.find(iter - tag_vector.begin()) == free_index_set.end()) {
			out->push_back(*iter);
		}
	}
}

void
TagList::GetTagById(const unsigned int id, Tag *out) const {
	*out = tag_vector[id];
}

void
TagList::GetTagByName(const QString& name, Tag *out) const {
	auto iter = tag_name_to_id_table.find(name);
	*out = tag_vector[iter.value()];
}

void
TagList::GetTagIdByName(const QString& name, unsigned int *id) const {
	auto iter = tag_name_to_id_table.find(name);
	*id = iter.value();
}

int
TagList::InsertTagMedia(const unsigned int tag_id, const unsigned int media_id) {
	tag_vector[tag_id].count++;
	tag_vector[tag_id].AddMediaId(media_id);
	return 1;
}

int
TagList::RemoveTagMedia(const unsigned int tag_id, const unsigned int media_id) {

	tag_vector[tag_id].RemoveMediaId(media_id);
	return 1;
}

int
TagList::UpdateTagName(const unsigned int tag_id, const QString& new_name) {

	Tag *tmp = &tag_vector[tag_id];
	tag_name_to_id_table.remove(tmp->name);
	tmp->name = new_name;
	tag_name_to_id_table.insert(new_name, tmp->id);

	return 1;
}

void
TagList::DumpTags(bool show_hole_entry) const {
	printf("Dumping Tag List:\n\n");
	if (Empty()) {
		printf("Empty\n");
		return;
	}

	for (auto iter = tag_vector.begin(); iter != tag_vector.end(); iter++) {
		if (iter->id == -1) {
			if (show_hole_entry) {
				printf("[X]\t");
			}
			else {
				continue;
			}
		}

		/*
		printf("\t%d Count: %d Name: %S\n", iter->id,
			iter->count,
			iter->name.data());*/
	}

	printf("Name table:\n");
	for (auto iter = tag_name_to_id_table.begin(); iter != tag_name_to_id_table.end(); iter++) {
		//printf("Name: %S\t\t-> %d\n", iter.key().data(),
			//iter.value());
	}
}