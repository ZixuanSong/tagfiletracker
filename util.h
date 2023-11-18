#pragma once
#include "media_structs.h"

#include <string>
#include <QString>
#include <Windows.h>

#define FILE_HASH_READ_BLOCK_SIZE	4096	//default ntfs logical block size

namespace PathUtil {

	//Split string into Directory and Filename given (C)-string (W)ide char pointer
	void SplitDirFilenameCW(const LPWSTR, const int, std::wstring&, std::wstring&);
	//Split string into Directory and Filename given std::(w)string
	void SplitDirFileNameW(const std::wstring&, std::wstring&, std::wstring&);

	bool FileExistsW(const std::wstring&);
	bool DirectoryExistsW(const std::wstring&);

	//returned dir does not have trailing slash. eg: C:\foo\bar instead of C:\foo\bar(\)
	int GetCurrentDirectoryToWString(std::wstring*);

	//given a root path and a new path, copy the relative part. no validity check
	int GetRelativePath(const std::wstring&, const std::wstring&, std::wstring*);

	int IsChildDir(std::wstring&, std::wstring&);
}

namespace FileUtil {
	int GetFileSHA2(const QString& abs_file_path, QString* hash_out);
	int GetFileSHA2(const QString& root_dir, const MediaInfo& media, QString* hash_out);			//spits out result in hex_hash
	int GetFileSHA2(const QString& root_dir, MediaInfo& media);									//the result is in media.hash
}