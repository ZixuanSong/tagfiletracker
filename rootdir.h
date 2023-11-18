#pragma once

/*
	RootDir: a class that represent a root directory out of the root directory tree
*/

/*
#include <Windows.h>
#include "db.h"

#define META_DIR_NAME L"\\.meta"

class RootDir {
public:
	explicit RootDir(const std::wstring&);
	explicit RootDir(LPWSTR, DWORD);

	int Init();

	int AddMedia(const Media&);
	int RemoveMedia(const Media&);

	inline bool MediaExist(const std::wstring& rel_wpath) {
		return (rel_wpath_lookup_table.find(rel_wpath) != rel_wpath_lookup_table.end());
	}

	inline std::wstring& GetWPath() {
		return abs_wpath;
	}

	//int GetMediaByAbsPath(const std::wstring&, Media*);
	int GetMediaByRelPath(const std::wstring&, Media*);
	int GetMediaPtrByRelPath(const std::wstring&, Media**);

	//recursively traverse this directory and all child directories then add all files to media list
	//should only be called when root dir database is created for the first time
	int Explore();

	//debug purpose
	void DumpMedia() const;

private:
	std::wstring								abs_wpath;
	std::list<Media>							media_list;
	std::unordered_map<std::wstring, Media*>	rel_wpath_lookup_table;

	MediaDatabase media_db;

	int InitMeta();
	int WalkDirectory(std::wstring&, std::queue<std::wstring>*);

	int PopulateDefaultMediaTable();
	int LoadMedia();
};*/