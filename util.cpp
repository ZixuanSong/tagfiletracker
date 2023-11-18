#include "pch.h"
#include "util.h"
#include "error.h"
#include "logger.h"
#include <memory>
#include <QCryptographicHash>
#include <QFile>
#include <QStringBuilder>

int
PathUtil::IsChildDir(std::wstring& root, std::wstring& target) {
	return 0;
}

void
PathUtil::SplitDirFilenameCW( const LPWSTR src, const int len, std::wstring& wdir, std::wstring& wname) {
	std::wstring wsrc(src, len);
	PathUtil::SplitDirFileNameW(wsrc, wdir, wname);
}

void 
PathUtil::SplitDirFileNameW(const std::wstring& wsrc, std::wstring& wdir, std::wstring& wname) {
	
	wdir.clear();
	wname.clear();

	size_t last_slash = wsrc.rfind(L'\\');

	if (last_slash != std::wstring::npos) {
		wdir = wsrc.substr(0, last_slash);
		wname = wsrc.substr(last_slash + 1, wsrc.size() - last_slash - 1);
		return;
	}

	wname.reserve(wsrc.size());
	wname = wsrc;
	//for (int i = 0; i < wsrc.size(); i++) {
		//wname.push_back(wsrc.at(i));
	//}
}

bool
PathUtil::FileExistsW(const std::wstring& wpath) {
	DWORD ret = GetFileAttributesW(wpath.c_str());
	if (ret == INVALID_FILE_ATTRIBUTES) {
		return false;
	}
	return true;
}

bool
PathUtil::DirectoryExistsW(const std::wstring& abs_wpath) {
	DWORD ret = GetFileAttributesW(abs_wpath.c_str());
	if (ret == INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	//not a directory
	if (!(ret & FILE_ATTRIBUTE_DIRECTORY)) {
		return false;
	}

	return true;
}

//ugly piece of shit. do it better
int
PathUtil::GetCurrentDirectoryToWString(std::wstring *out) {
	WCHAR buffer[MAX_PATH];
	DWORD ret = GetCurrentDirectory(MAX_PATH, buffer);
	if (ret == 0) {
		return -1;
	}

	if (ret > MAX_PATH) {
		//allocated larger buffer
		std::unique_ptr<WCHAR[]> new_buf = std::make_unique<WCHAR[]>(ret);

		DWORD ret2 = GetCurrentDirectory(ret, new_buf.get());
		if (ret2 == 0) {
			//failed for good
			return -1;
		}

		out->assign(new_buf.get(), ret2);
		return 1;
	}

	out->assign(buffer, ret);
	return 1;
}

int
PathUtil::GetRelativePath(const std::wstring& abs_root_path, const std::wstring& abs_sub_path, std::wstring* result) {
	if (abs_root_path.size() == abs_sub_path.size()) {
		//resulting relative path is simply nothing
		result->clear();
		return 1;
	}

	size_t result_len = abs_sub_path.size() - abs_root_path.size();
	result->reserve(result_len);
	result->assign(abs_sub_path, abs_root_path.size(), result_len);
	return 1;
}

int
FileUtil::GetFileSHA2(const QString& abs_file_path, QString* hash_out) {
	QCryptographicHash hasher(QCryptographicHash::Sha256);
	QFile file(abs_file_path);
	QByteArray buff;

	if (!file.open(QIODevice::ReadOnly)) {
		return -Error::QFILE_OPEN;
		Logger::Log(QFILE_OPEN_MSG, LogEntry::LT_ERROR);
	}

	while (!file.atEnd()) {
		buff = file.read(FILE_HASH_READ_BLOCK_SIZE);
		hasher.addData(buff);
	}

	buff = std::move(hasher.result());
	*hash_out = std::move(buff.toHex());

	file.close();
	return 1;
}

int
FileUtil::GetFileSHA2(const QString& root_dir, const MediaInfo& media, QString* hash_out) {

	QString full_path = root_dir % media.GetSubpathLongName();

	Logger::Log("Calculating hash for " % media.long_name);

	return FileUtil::GetFileSHA2(full_path, hash_out);
}

int
FileUtil::GetFileSHA2(const QString& root_dir, MediaInfo& media) {

	media.hash.clear();
	return GetFileSHA2(root_dir, media, &media.hash);
}