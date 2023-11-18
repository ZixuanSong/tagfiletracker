/*
#include "pch.h"

#include "rootdir.h"
#include "util.h"


RootDir::RootDir(const std::wstring& wpath) :
	abs_wpath(wpath)
{
}

RootDir::RootDir(LPWSTR str, DWORD len) {
	abs_wpath.assign(str, len);
}

int
RootDir::Init() {
	int ret = InitMeta();
	if (ret < 0) {
		//something wrong with initializing meta data
		return -1;
	}
	return 1;
}

int
RootDir::AddMedia(const Media& media) {
	media_list.push_back(media);

	//fill in look up table
	std::wstring tmp;
	// media relative size + file name + extra '\' between relative path and file name
	tmp.reserve(media.sub_wpath.size() + media.name.size() + 1);
	tmp.append(media.sub_wpath);
	tmp.push_back(L'\\');
	tmp.append(media.name);

	rel_wpath_lookup_table.insert({ tmp, &media_list.back() });

	return 1;
}

int
RootDir::RemoveMedia(const Media& media) {
	return 1;
}


int
RootDir::GetMediaByAbsPath(const std::wstring& abs_wpath, Media *media) {
	auto iter = abs_wpath_lookup_table.find(abs_wpath);
	if (iter == abs_wpath_lookup_table.end()) {
		//media not found
		return -1;
	}

	//nullptr means don't want to actually copy content. this is used as exsistence check
	if (media == nullptr) {
		return 1;
	}

	*media = *(iter->second);
	return 1;
}

int
RootDir::GetMediaByRelPath(const std::wstring& rel_wpath, Media *media) {
	
	std::wstring new_wpath(rel_wpath);
	new_wpath.reserve(abs_wpath.size() + rel_wpath.size());
	new_wpath.assign(abs_wpath);
	new_wpath.append(rel_wpath);
	

	*media = *(rel_wpath_lookup_table.find(rel_wpath)->second);
	return 1;
}

int 
RootDir::GetMediaPtrByRelPath(const std::wstring& rel_wpath, Media** ptr) {
	*ptr = rel_wpath_lookup_table.find(rel_wpath)->second;
	return 1;
}
int
RootDir::Explore() {

	std::queue<std::wstring> dir_queue;

	dir_queue.push(abs_wpath + L"\\*");

	int ret;
	std::wstring curr_dir;

	while (!dir_queue.empty()) {

		curr_dir = dir_queue.front();
		dir_queue.pop();

		//printf("At dir: %S\n", curr_dir.c_str());

		ret = WalkDirectory(curr_dir, &dir_queue);
		if (ret < 0) {
			//something wrong with walking this directory
			return -1;
		}
	}
	return 1;
}

void
RootDir::DumpMedia() const {
	printf("Dumping Media List:\n");
	if (media_list.empty()) {
		printf("Empty\n");

		//return here because if list is empty table would be empty too
		return;
	}

	for (auto iter = media_list.begin(); iter != media_list.end(); iter++) {
		printf("Rel Path: \t%S\nName: \t\t%S\nAlt name: \t%S\n",	iter->sub_wpath.c_str(),
																	iter->name.c_str(),
																	iter->alt_name.c_str());
	}

	printf("Dumping abs wpath lookup table\n");
	if (rel_wpath_lookup_table.empty()) {
		printf("Empty\n");
		return;
	}

	for (auto iter = rel_wpath_lookup_table.begin(); iter != rel_wpath_lookup_table.end(); iter++) {
		printf("%S\t->\t%p(%S)\n",		iter->first.c_str(),
										iter->second,
										iter->second->name.c_str());
	}
}

int
RootDir::InitMeta() {

	int ret;
	std::wstring tmp_str(abs_wpath);

	//if metadata directory doesn't exist then create one
	tmp_str.append(META_DIR_NAME);
	if (!PathUtil::DirectoryExistsW(tmp_str)) {
		ret = CreateDirectoryW(tmp_str.c_str(), NULL);
		if (ret == 0) {
			//can't create directory
			return -1;
		}
	}

	//if local media database dosen't exist create one
	//tmp_str.append(ROOTDIR_DB_PATH);
	if (!PathUtil::FileExistsW(tmp_str)) {

		media_db.SetPathW(tmp_str);
		ret = PopulateDefaultMediaTable();
		if (ret < 0) {
			//can't create default media database
			return -2;
		}

		//explore this root dir
		ret = Explore();
		if (ret < 0) {
			//something wrong with explore
			return -3;
		}
	}
	else {
		ret = LoadMedia();
		if (ret < 0) {
			//can't load media
			return -4;
		}
	}

	return 1;
}

int
RootDir::WalkDirectory(std::wstring& wpath, std::queue<std::wstring>* dir_queue) {

	WIN32_FIND_DATA file_data;
	Media media;

	//path without the * and \ at the end. ex. if wpath: C:\dir1\di2\* then sanitized is C:\dir1\di2
	std::wstring sanitized_path(wpath, 0, wpath.size() - 2);

	HANDLE handle = FindFirstFileW(wpath.c_str(), &file_data);
	if (handle == INVALID_HANDLE_VALUE) {
		//invalid handle
		return -1;
	}

	do {
		//do not care about symlink
		if (file_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
			file_data.dwReserved0 & IO_REPARSE_TAG_SYMLINK) {
			continue;
		}

		//do not care about hidden file
		if (file_data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
			continue;
		}

		//do not care about files/directories begin with ".", they are hidden by naming convention
		if (file_data.cFileName[0] == L'.') {
			continue;
		}

		if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

			//remove '*' from current search direcetory path
			std::wstring new_dir_path(wpath, 0, wpath.size() - 1);

			//append traversed dir name then append \*
			new_dir_path.append(file_data.cFileName);
			new_dir_path.append(L"\\*");

			//add to dir queue to be walked
			dir_queue->push(new_dir_path);

			//printf("Added dir: %S\n", new_dir_path.c_str());
			//printf("DIR: %S(%S)\n", file_data.cFileName, file_data.cAlternateFileName);
			continue;
		}

		media.name = file_data.cFileName;
		media.alt_name = file_data.cAlternateFileName;
		PathUtil::GetRelativePath(abs_wpath, sanitized_path, &media.sub_wpath);
		AddMedia(media);

		//printf("%S(%S)\n", file_data.cFileName, file_data.cAlternateFileName);

	} while (FindNextFileW(handle, &file_data) != 0);

	if (GetLastError() != ERROR_NO_MORE_FILES) {
		//error with listing files
		return -2;
	}

	FindClose(handle);
	return 1;
}

int
RootDir::PopulateDefaultMediaTable() {
	int ret = media_db.Open();
	if (ret < 0) {
		//can't open
		return -1;
	}

	//ret = media_db.CreateTable();
	if (ret < 0) {
		//can't create tag table
		return -2;
	}
	media_db.Close();
	return 1;
}

int
RootDir::LoadMedia() {
	//loading is walking fornow
	Explore();

	
	int ret = media_db.Open();
	if (ret < 0) {
		//can't open
		return -1;
	}

	ret = media_db.GetAllRows(&media_list);
	if (ret < 0) {
		//can't load
		return -2;
	}

	media_db.Close();
	

	return 1;
}
*/