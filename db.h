#pragma once

/*
	Wrapper / Library that handles all calls to sqlite3 library

	One database per table:

	Media Table
	- ID: Integer
	- Dir: STR			Only the absolute path to directory no trailing \
	- Full_Name: STR
	- Short_name: STR	the alternate name
	- Hash: CHAR(64)	Hash of media

	Tag Table
	- ID: Integer
	- Count: Integer
	- Name: Text

	Tag Link Table
	- ID: Integer
	- Tag ID: Integer
	- Media ID: Integer

*/

#include "sqlite3.h"
#include "tag_structs.h"
#include "tag_list.h"
#include "media_structs.h"
#include "media_list.h"
#include "tag_link.h"
#include "logger.h"
#include <QString>
#include <string>
#include <list>
#include <unordered_map>
#include <Windows.h>

#define TAGS_DB_NAME "tags.sqlite3"
#define TAGLINK_DB_NAME "tag_links.sqlite3"
#define MEDIA_DB_NAME "media.sqlite3"

/*
	Database classes are treated like libraries which interfaces with 
	internals of sqlite
*/

/*
	Design Decision:
	
	1. Whose job is it to to check if database handles are valid?
	Since Database classes are supposed to call sqlite internals. The quesiton
	boils down to is openning and closing sqlite internal? I think the answer is
	yes. openning and closing is purely implemented  

	2. Do we need to check error code returned by sqlite3_finalize?
	No. Since error will only indicate the evaluation of the statement and not
	the status of the finalize functions. If the evaluation failed then an error
	should be indicated earlier.
*/

class Database {
public:

	Database();
	virtual ~Database();

	//void SetPathW(std::wstring &);
	//void SetPathC(LPWSTR, DWORD);

	//void GetPathW(std::wstring*);

	void SetPath(const QString& new_path);
	QString GetPath();

	int Init();

	bool Exist() const;

	bool Opened() const;

	int Open();
	
	int Close();

	//int Flush();

	int LastRowId();

	int SingleStepQuery(const QString& query);

	int MultiStepQuery(const QString& query, std::function<void(sqlite3_stmt* statement)> statement_result_handler);

	int SingleStepMultiStatementQuery(const QString& query);
	
	virtual int	CreateDefaultTable() = 0;

protected:
	sqlite3*		db_handle;	//nullptr if not opened
	QString			db_path;
	Logger*			logger;

	void LogSQLError(const QString& err_text, int sqlite_err_no);
};

class TagDatabase : public Database {
public:

	TagDatabase();

	int CreateDefaultTable() override;
	int GetAllTags(TagList*);
	int GetTagById(unsigned int, Tag*);
	int InsertTag(const Tag&);
	int UpdateTag(const Tag&);
	int RemoveTag(const unsigned int);
};

class TagLinkDatabase : public Database {
public:

	TagLinkDatabase();

	int CreateDefaultTable() override;
	int GetAllTagLinks(QVector<TagLink>*);
	int CreateTagLink(const unsigned int tag_id, const unsigned int media_id);
	int CreateTagLinkByTagMediaIdList(const QVector<unsigned int>& tag_id_List, const QVector<unsigned int>& media_id_list);
	int RemoveTagLinkByTagIdMediaId(const unsigned int, const unsigned int);
	int RemoveTagLinkByTagId(const unsigned int);
	int RemoveTagLinkByMediaId(const unsigned int);
	int RemoveTagLinkByMediaIdList(const QVector<unsigned int>& media_id_list);
};

class MediaDatabase : public Database {
public:

	MediaDatabase();

	int CreateDefaultTable() override;
	int GetAllMedia(QVector<MediaInfo>* media_vec);
	int InsertMedia(const MediaInfo& new_media);
	int	InsertMediaList(const QVector<MediaInfo>& new_media_list);
	int UpdateMedia(const MediaInfo&);
	int UpdateMediaList(const QVector<MediaInfo>& media_list);
	int RemoveMedia(const unsigned int);
	int RemoveMediaList(const QVector<unsigned int>& media_id_list);
};