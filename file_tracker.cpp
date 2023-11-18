#include "file_tracker.h"
#include "error.h"
#include <Windows.h>
#include <QQueue>
#include <QStringBuilder>
//#include <qdebug.h>

int
FileTracker::GetFileLongShortName(const QString& abs_path, QString* long_name, QString* short_name) {
	//find both long name and alt name
	WIN32_FIND_DATAW winapi_file_data;
	std::wstring wfull_path = abs_path.toStdWString();
	HANDLE find_file_handle = FindFirstFileW(wfull_path.data(), &winapi_file_data);
	if (find_file_handle == INVALID_HANDLE_VALUE) {
		return -1;
	}

	*short_name = QString::fromStdWString(winapi_file_data.cAlternateFileName);
	*long_name = QString::fromStdWString(winapi_file_data.cFileName);

	FindClose(find_file_handle);
	return 1;
}

int 
FileTracker::IsDir(const QString& abs_path, bool *out) {
	WIN32_FIND_DATAW winapi_file_data;
	std::wstring wfull_path = abs_path.toStdWString();
	HANDLE find_file_handle = FindFirstFileW(wfull_path.data(), &winapi_file_data);
	if (find_file_handle == INVALID_HANDLE_VALUE) {
		return -1;
	}

	*out = winapi_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

	FindClose(find_file_handle);
	return 1;
}


FileTracker::FileTracker()
{
}

FileTracker::~FileTracker()
{
}


void 
FileTracker::SetRootDir(const QString& path) {
	dir_tree.root_dir_abs_path = path;

	FileTracker::GetFileLongShortName(path, &dir_tree.root_dir_node.long_name, &dir_tree.root_dir_node.short_name);
}

int
FileTracker::AddDirAbsPath(const QString& parent_abs_path, const QString& long_name, const QString& short_name) {
	
	return AddDirSubPath(parent_abs_path.mid(dir_tree.root_dir_abs_path.size()), long_name, short_name);
}

int		
FileTracker::AddDirSubPath(const QString& sub_path, const QString& long_name, const QString& short_name) {
	DirTreeNode *node_ptr;

	if (sub_path.size() > 0) {
		if (GetDirNodePtr(sub_path, &node_ptr) < 0) {
			return -1;
		}
	}
	else {
		node_ptr = &dir_tree.root_dir_node;
	}


	std::unique_ptr<DirTreeNode> new_node = std::make_unique<DirTreeNode>();

	new_node->long_name = long_name;
	new_node->short_name = short_name;

	node_ptr->child_dir_name_to_node_table.insert(long_name, new_node.get());

	if (short_name.size() != 0) {
		node_ptr->child_dir_short_name_to_node_table.insert(short_name, new_node.get());
	}

	node_ptr->child_dir_list.push_back(std::move(new_node));
	
	return 1;
}

int		
FileTracker::AddMediaAbsPath(const QString& abs_path, const unsigned int media_id) {
	return AddMediaSubPath(abs_path.mid(dir_tree.root_dir_abs_path.size()), media_id);
}

int
FileTracker::AddMediaSubPath(const QString& sub_path, const unsigned int media_id) {
	DirTreeNode *node_ptr;

	if (sub_path.size() > 0) {
		if (GetDirNodePtr(sub_path, &node_ptr) < 0) {
			return -1;
		}
	}
	else {
		node_ptr = &dir_tree.root_dir_node;
	}

	node_ptr->media_id_set.insert(media_id);

	return 1;
}

int		
FileTracker::RemoveMedia(const QString& sub_path, const unsigned int media_id) {
	DirTreeNode *node_ptr;

	if (sub_path.size() > 0) {
		if (GetDirNodePtr(sub_path, &node_ptr) < 0) {
			return -1;
		}
	}
	else {
		node_ptr = &dir_tree.root_dir_node;
	}

	node_ptr->media_id_set.remove(media_id);

	return 1;
}

int		
FileTracker::UpdateDirName(const QString& sub_path_name, const QString& long_name, const QString& short_name) {
	
	QString old_name;
	QString parent_sub_path;
	DirTreeNode *parent_node_ptr;
	DirTreeNode *dir_node_ptr;

	//divide parent sub path and name
	parent_sub_path = sub_path_name;
	old_name = sub_path_name.mid(sub_path_name.lastIndexOf("\\") + 1);
	parent_sub_path.chop(old_name.size() + 1);	//plus 1 to chop off the \ separator

	//get parent node ptr
	if (GetDirNodePtr(parent_sub_path, &parent_node_ptr) < 0) {
		return -1;
	}

	//get the dir node ptr
	if (parent_node_ptr->child_dir_name_to_node_table.contains(old_name)) {
		dir_node_ptr = parent_node_ptr->child_dir_name_to_node_table.value(old_name);
	}
	else {
		dir_node_ptr = parent_node_ptr->child_dir_short_name_to_node_table.value(old_name);
	}

	//remove old look up table
	parent_node_ptr->child_dir_name_to_node_table.remove(dir_node_ptr->long_name);
	if (dir_node_ptr->short_name.size() > 0) {
		parent_node_ptr->child_dir_short_name_to_node_table.remove(dir_node_ptr->short_name);
	}

	//rename node
	dir_node_ptr->long_name = long_name;
	dir_node_ptr->short_name = short_name;

	//add new look up table
	parent_node_ptr->child_dir_name_to_node_table.insert(long_name, dir_node_ptr);
	if (short_name.size() > 0) {
		parent_node_ptr->child_dir_short_name_to_node_table.insert(short_name, dir_node_ptr);
	}

	return 1;
}

int		
FileTracker::UpdateDirSubdir(const QString& sub_path_name, const QString& new_sub_dir) {

	QString old_name;
	QString parent_sub_path;
	DirTreeNode *parent_node_ptr;
	DirTreeNode *new_parent_node_ptr;
	DirTreeNode *dir_node_ptr;

	//divide parent sub path and name
	parent_sub_path = sub_path_name;
	old_name = sub_path_name.mid(sub_path_name.lastIndexOf("\\") + 1);
	parent_sub_path.chop(old_name.size() + 1);	//plus 1 to chop off the \ separator

	//get parent node ptr
	if (GetDirNodePtr(parent_sub_path, &parent_node_ptr) < 0) {
		return -1;
	}

	//get the dir node ptr
	if (parent_node_ptr->child_dir_name_to_node_table.contains(old_name)) {
		dir_node_ptr = parent_node_ptr->child_dir_name_to_node_table.value(old_name);
	}
	else {
		dir_node_ptr = parent_node_ptr->child_dir_short_name_to_node_table.value(old_name);
	}

	//remove old look up table
	parent_node_ptr->child_dir_name_to_node_table.remove(dir_node_ptr->long_name);
	if (dir_node_ptr->short_name.size() > 0) {
		parent_node_ptr->child_dir_short_name_to_node_table.remove(dir_node_ptr->short_name);
	}

	//get new parent node
	if (GetDirNodePtr(new_sub_dir, &new_parent_node_ptr) < 0) {
		return -2;
	}

	//add lookup table to new parent
	new_parent_node_ptr->child_dir_name_to_node_table.insert(dir_node_ptr->long_name, dir_node_ptr);
	if (dir_node_ptr->short_name.size() > 0) {
		new_parent_node_ptr->child_dir_short_name_to_node_table.insert(dir_node_ptr->short_name, dir_node_ptr);
	}

	//find and move unique ptr

	auto iter = parent_node_ptr->child_dir_list.begin();
	for (; iter != parent_node_ptr->child_dir_list.end(); iter++) {
		if (iter->get() == dir_node_ptr) {
			new_parent_node_ptr->child_dir_list.push_back(std::move(*iter));
			break;
		}
	}

	//delete old unique ptr
	parent_node_ptr->child_dir_list.erase(iter);

	return 1;
}

int		
FileTracker::RemoveDir(const QString& sub_path_name) {
	
	QString old_name;
	QString parent_sub_path;
	DirTreeNode *parent_node_ptr;
	DirTreeNode *dir_node_ptr;

	//divide parent sub path and name
	parent_sub_path = sub_path_name;
	old_name = sub_path_name.mid(sub_path_name.lastIndexOf("\\") + 1);
	parent_sub_path.chop(old_name.size() + 1);	//plus 1 to chop off the \ separator

	//get parent node ptr
	if (GetDirNodePtr(parent_sub_path, &parent_node_ptr) < 0) {
		return -1;
	}

	//get the dir node ptr
	if (parent_node_ptr->child_dir_name_to_node_table.contains(old_name)) {
		dir_node_ptr = parent_node_ptr->child_dir_name_to_node_table.value(old_name);
	}
	else {
		dir_node_ptr = parent_node_ptr->child_dir_short_name_to_node_table.value(old_name);
	}

	//remove old look up table
	parent_node_ptr->child_dir_name_to_node_table.remove(dir_node_ptr->long_name);
	if (dir_node_ptr->short_name.size() > 0) {
		parent_node_ptr->child_dir_short_name_to_node_table.remove(dir_node_ptr->short_name);
	}

	//find and remove dir node
	auto iter = parent_node_ptr->child_dir_list.begin();
	for (; iter != parent_node_ptr->child_dir_list.end(); iter++) {
		if (iter->get() == dir_node_ptr) {
			break;
		}
	}

	parent_node_ptr->child_dir_list.erase(iter);
	return 1;
}

bool
FileTracker::DirExist(const QString& rel_path) {
	
	DirTreeNode *node_ptr;
	if (GetDirNodePtr(rel_path, &node_ptr) < 0) {
		return false;
	}

	return true;
}

bool
FileTracker::DirExistAbs(const QString& abs_path) {
	return DirExist(abs_path.mid(dir_tree.root_dir_abs_path.size()));
}

void 
FileTracker::GetPathLongName(const QString& path, QString* out) {
	QStringList sub_path_component_list = path.split("\\", QString::SkipEmptyParts);

	out->clear();
	DirTreeNode *curr_node = &dir_tree.root_dir_node;

	for (QString path_component : sub_path_component_list) {

		out->append('\\');

		if (curr_node->child_dir_name_to_node_table.contains(path_component)) {
			curr_node = curr_node->child_dir_name_to_node_table.value(path_component);
			out->append(curr_node->long_name);

		} else {
			curr_node = curr_node->child_dir_short_name_to_node_table.value(path_component);
			out->append(curr_node->long_name);
		}
	}
}

int
FileTracker::GetDirMediaIdRecurByPath(const QString& path, QVector<unsigned int>* out) {
	DirTreeNode *dir_node;

	//get dir node
	if (GetDirNodePtr(path, &dir_node) < 0) {
		return -1;
	}

	return GetDirMediaIdRecurByNode(dir_node, out);
}

void
FileTracker::Clear() {

	dir_tree.root_dir_node.child_dir_list.clear();
	dir_tree.root_dir_node.child_dir_name_to_node_table.clear();
	dir_tree.root_dir_node.child_dir_short_name_to_node_table.clear();
}

//private

int	
FileTracker::GetDirNodePtr(const QString& sub_path, DirTreeNode** out) {
	QStringList path_component_list = sub_path.split('\\', QString::SkipEmptyParts);

	DirTreeNode *curr_node = &dir_tree.root_dir_node;
	for (QString component : path_component_list) {
		if (curr_node->child_dir_name_to_node_table.contains(component)) {
			curr_node = curr_node->child_dir_name_to_node_table.value(component);
			continue;
		}

		if (curr_node->child_dir_short_name_to_node_table.contains(component)) {
			curr_node = curr_node->child_dir_short_name_to_node_table.value(component);
			continue;
		}

		//this path leads to non existing dir
		return -1;
	}

	*out = curr_node;
	return 1;
}

int 
FileTracker::GetDirMediaIdRecurByNode(DirTreeNode *root_node, QVector<unsigned int>* out) {
	//BFS all child dirs

	QQueue<DirTreeNode*> node_queue;
	DirTreeNode *curr_node;

	node_queue.enqueue(root_node);

	while (!node_queue.empty()) {
		curr_node = node_queue.dequeue();

		//enqueue child dirs
		for (auto iter = curr_node->child_dir_list.begin(); iter != curr_node->child_dir_list.end(); iter++) {
			node_queue.enqueue(iter->get());
		}

		if (curr_node->media_id_set.empty()) {
			continue;
		}

		out->reserve(out->size() + curr_node->media_id_set.size());
		out->append(curr_node->media_id_set.toList().toVector());
	}

	return 1;
}

int		
FileTracker::GetDirName(const QString& sub_path_name, QString* long_name, QString* short_name) {
	DirTreeNode *dir_node;

	//get dir node
	if (GetDirNodePtr(sub_path_name, &dir_node) < 0) {
		return -1;
	}

	*long_name = dir_node->long_name;
	*short_name = dir_node->short_name;
	
	return 1;
}