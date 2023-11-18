#include "pch.h"

#include "db.h"
#include "util.h"
#include "error.h"

#include <QStringBuilder>
#include <memory>

Database::Database() :
	db_handle(nullptr)
{
}

Database::~Database() {
	if (db_handle != nullptr) {
		sqlite3_close(db_handle);
	}
}

void
Database::SetPath(const QString& new_path) {
	db_path = new_path;
}

QString
Database::GetPath() {
	return db_path;
}

int 
Database::Init() {
	
	bool new_db_flag = QFile(db_path).exists();
	
	if (Open() < 0) {
		return -Error::DB_OPEN;
	}

	//create default if this file is new
	if (!new_db_flag) {
		if (CreateDefaultTable() < 0) {
			Logger::Log(DB_DEFAULT_TABLE_MSG, LogEntry::LT_ERROR);
			return -Error::DB_DEFAULT_TABLE;
		}
	}

	return 1;
}

bool
Database::Exist() const {
	return PathUtil::FileExistsW(db_path.toStdWString());
}

bool 
Database::Opened() const {
	return db_handle != nullptr;
}

int
Database::Open() {
	if (Opened()) {
		return 1;
	}

	std::wstring tmp = db_path.toStdWString();
	int ret = sqlite3_open16(tmp.c_str(), &db_handle);
	if (ret != SQLITE_OK) {
		LogSQLError(DB_OPEN_MSG, ret);
		return -Error::DB_OPEN;
	}

	return 1;
}

int 
Database::Close() {
	int ret = sqlite3_close(db_handle);
	
	//set handle to nullptr always happens regardless success or failure
	db_handle = nullptr;

	//couldnt close
	if (ret != SQLITE_OK) {
		//this should never happen. close is called during operation clean up
		//the clean up shouldn't fail or else how do you clean up a clean up?
		//make sure to free all outstanding statements before close is called
		return -1;
	}

	return 1;
}

//this is used for mid transaction only
/*
int
Database::Flush() {
	if (sqlite3_db_cacheflush(db_handle) != SQLITE_OK) {
		return -1;
	}
	return 1;
}*/

int
Database::LastRowId() {
	return (int) sqlite3_last_insert_rowid(db_handle);
}

int 
Database::SingleStepQuery(const QString& query) {
	sqlite3_stmt *statement;

	//since sqlite3 takes wchar array we convert qstring to this array
	std::unique_ptr<wchar_t[]> wchar_array_buff = std::make_unique<wchar_t[]>(query.size());
	query.toWCharArray(wchar_array_buff.get());

	//sizeof wchar array in bytes
	int byte_length = query.size() * sizeof(wchar_t);

	//point to unused portion of query 
	//not a big deal because there's only one query
	const void *unused_begin;

	int ret = sqlite3_prepare16_v2(db_handle, wchar_array_buff.get(), byte_length, &statement, &unused_begin);
	if (ret != SQLITE_OK) {
		LogSQLError(DB_STATEMENT_PREPARE_MSG, ret);
		return -Error::DB_STATEMENT_PREPARE;
	}

	ret = sqlite3_step(statement);
	if (ret != SQLITE_DONE) {

		LogSQLError(DB_STATEMENT_STEP_MSG, ret);
		return -Error::DB_STATEMENT_STEP;
	}

	//finalize could error but there's no recovering from it: not to clean up a failed clean up
	ret = sqlite3_finalize(statement);
	return 1;
}

int 
Database::MultiStepQuery(const QString& query, std::function< void(sqlite3_stmt* statement) > statement_result_handler) {
	sqlite3_stmt *statement;

	//since sqlite3 takes wchar array we convert qstring to this array
	std::unique_ptr<wchar_t[]> wchar_array_buff = std::make_unique<wchar_t[]>(query.size());
	query.toWCharArray(wchar_array_buff.get());

	//sizeof wchar array in bytes
	int byte_length = query.size() * sizeof(wchar_t);

	//point to unused portion of query 
	//not a big deal because there's only one query
	const void *unused_begin;

	int ret = sqlite3_prepare16_v2(db_handle, wchar_array_buff.get(), byte_length, &statement, &unused_begin);
	if (ret != SQLITE_OK) {
		LogSQLError(DB_STATEMENT_PREPARE_MSG, ret);
		return -Error::DB_STATEMENT_PREPARE;
	}

	for (;;) {
		ret = sqlite3_step(statement);

		//stepped through all possible results
		if (ret == SQLITE_DONE) {
			break;
		}

		//no more rows to process but not done -> error
		if (ret != SQLITE_ROW) {
			LogSQLError(DB_STATEMENT_STEP_MSG, ret);
			return -Error::DB_STATEMENT_STEP;
		}

		//process the result
		statement_result_handler(statement);
	}

	//finalize could error but there's no recovering from it: not to clean up a failed clean up
	sqlite3_finalize(statement);
	return 1;
}

int 
Database::SingleStepMultiStatementQuery(const QString& query) {

	char *errmsg_ptr;

	//since sqlite3 takes wchar array we convert qstring to this array
	std::string std_query = query.toStdString();

	int ret = sqlite3_exec(db_handle, std_query.c_str(), NULL, NULL, &errmsg_ptr);
	if (ret != SQLITE_OK) {
		LogSQLError(DB_EXEC_MSG, ret);
		LogSQLError(QString::fromLocal8Bit(errmsg_ptr), ret);

		//errmsg was allocated using sqlite3_alloc, free it with sqlite3_free
		sqlite3_free(errmsg_ptr);
		return -Error::DB_EXEC;
	}

	return 1;
}


//private

void 
Database::LogSQLError(const QString& err_text, int sqlite_err_no) {

	const char *sqlite_err_text = sqlite3_errstr(sqlite_err_no);
	Logger::Log(err_text % ": " % QString::fromUtf8(sqlite_err_text) % " (" % QString::number(sqlite_err_no) % ')',  LogEntry::LT_ERROR);
}

//Tag Database

TagDatabase::TagDatabase()
{
}

int
TagDatabase::CreateDefaultTable() {

	const QString query =	"CREATE TABLE TAGS("
							"id		INTEGER		PRIMARY KEY		NOT NULL,"
							"count	INT							NOT NULL,"
							"name	TEXT						NOT NULL);";

	return SingleStepQuery(query);
}

int 
TagDatabase::GetAllTags(TagList* tag_list) {
	
	const QString query = "SELECT * FROM TAGS;";
	Tag tmp;
	return MultiStepQuery(query, [&tmp, tag_list](sqlite3_stmt* statement) {
		
		tmp.id = sqlite3_column_int(statement, 0);
		tmp.count = sqlite3_column_int(statement, 1);
		tmp.name = QString::fromWCharArray((WCHAR*)	sqlite3_column_text16(statement, 2),
													sqlite3_column_bytes16(statement, 2) >> 1);

		tag_list->InsertSavedTag(tmp);
	});
}

int 
TagDatabase::InsertTag(const Tag& new_tag) {

	//QString sanitized_name = new_tag.name;
	//sanitized_name.replace('\'', "''");

	const QString query = "INSERT INTO TAGS VALUES(" % QString::number(new_tag.id) % ',' % QString::number(new_tag.count) % ",'" % new_tag.name % "');";
	return SingleStepQuery(query);
}

int
TagDatabase::UpdateTag(const Tag& tag){
	
	//QString sanitized_name = tag.name;
	//sanitized_name.replace('\'', "''");

	const QString query = "UPDATE TAGS SET count = " % QString::number(tag.media_id_list.size()) % ", name = '" % tag.name % "' WHERE id = " % QString::number(tag.id) % ";";
	return SingleStepQuery(query);
}

int
TagDatabase::RemoveTag(const unsigned int tag_id) {
	const QString query = "DELETE FROM TAGS WHERE id = " % QString::number(tag_id) % ';';
	return SingleStepQuery(query);
}


//TagLink Database

TagLinkDatabase::TagLinkDatabase()
{
}

int
TagLinkDatabase::CreateDefaultTable() {
	const QString query =	"CREATE TABLE TAG_LINKS("
							"id				INTEGER		PRIMARY KEY,"
							"tag_id			INT							NOT NULL,"
							"media_id		INT							NOT NULL);";

	return SingleStepQuery(query);
}

int
TagLinkDatabase::GetAllTagLinks(QVector<TagLink>* taglink_list) {
	const QString query = "SELECT * FROM TAG_LINKS;";
	
	TagLink tmp;
	return MultiStepQuery(query, [&tmp, taglink_list](sqlite3_stmt* statement) {

		tmp.id = sqlite3_column_int(statement, 0);
		tmp.tag_id = sqlite3_column_int(statement, 1);
		tmp.media_id = sqlite3_column_int(statement, 2);

		taglink_list->push_back(tmp);
	});
}

int
TagLinkDatabase::CreateTagLink(const unsigned int tag_id, const unsigned int media_id) {
	const QString query = "INSERT INTO TAG_LINKS (tag_id, media_id) VALUES(" % QString::number(tag_id) % ", " % QString::number(media_id) % ");";
	return SingleStepQuery(query);
}


//tag_id_list and media_id_list should be the same length 1:1 map
int 
TagLinkDatabase::CreateTagLinkByTagMediaIdList(const QVector<unsigned int>& tag_id_list, const QVector<unsigned int>& media_id_list) {
	const QString transaction_format_str = "INSERT INTO TAG_LINKS (tag_id, media_id) VALUES(%1, %2);";
	QString query = "BEGIN TRANSACTION;";


	for (int i = 0; i < tag_id_list.size(); i++) {
		query.append(transaction_format_str.arg(QString::number(tag_id_list[i]), QString::number(media_id_list[i])));
	}

	query.append("COMMIT;");

	return SingleStepMultiStatementQuery(query);
}

int 
TagLinkDatabase::RemoveTagLinkByTagIdMediaId(const unsigned int tag_id, const unsigned int media_id) {
	const QString query = "DELETE FROM TAG_LINKS WHERE (tag_id = " % QString::number(tag_id) % " AND media_id = " % QString::number(media_id) % ");";
	return SingleStepQuery(query);
}

int
TagLinkDatabase::RemoveTagLinkByTagId(const unsigned int tag_id) {
	const QString query = "DELETE FROM TAG_LINKS WHERE tag_id = " % QString::number(tag_id) % ';';
	return SingleStepQuery(query);
}

int
TagLinkDatabase::RemoveTagLinkByMediaId(const unsigned int media_id) {
	const QString query = "DELETE FROM TAG_LINKS WHERE media_id = " % QString::number(media_id) % ';';
	return SingleStepQuery(query);
}

int 
TagLinkDatabase::RemoveTagLinkByMediaIdList(const QVector<unsigned int>& media_id_list) {
	const QString transaction_format_str = "DELETE FROM TAG_LINKS WHERE media_id = %1;";
	QString query = "BEGIN TRANSACTION;";


	for (auto iter = media_id_list.begin(); iter < media_id_list.end(); iter++) {
		query.append(transaction_format_str.arg(QString::number(*iter)));
	}

	query.append("COMMIT;");

	return SingleStepMultiStatementQuery(query);
}

//Media database

MediaDatabase::MediaDatabase() 
{
}

int
MediaDatabase::CreateDefaultTable() {

	const QString query =	"CREATE TABLE MEDIA("
							"id			INTEGER			PRIMARY KEY,"
							"sub_path	TEXT							NOT NULL,"
							"name		TEXT							NOT NULL,"
							"alt_name	TEXT							NOT NULL,"
							"hash		CHARACTER(64)					NOT NULL);";

	return SingleStepQuery(query);
}

int 
MediaDatabase::GetAllMedia(QVector<MediaInfo> *media_list) {

	const QString query = "SELECT * FROM MEDIA;";

	MediaInfo tmp;
	return MultiStepQuery(query, [&tmp, media_list](sqlite3_stmt* statement) {


		tmp.id = sqlite3_column_int(statement, 0);
		tmp.sub_path = QString::fromWCharArray((WCHAR*)sqlite3_column_text16(statement, 1),
			sqlite3_column_bytes16(statement, 1) >> 1);

		tmp.long_name = QString::fromWCharArray((WCHAR*)sqlite3_column_text16(statement, 2),
			sqlite3_column_bytes16(statement, 2) >> 1);

		tmp.short_name = QString::fromWCharArray((WCHAR*)sqlite3_column_text16(statement, 3),
			sqlite3_column_bytes16(statement, 3) >> 1);
		
		tmp.hash = QString::fromLocal8Bit((char*)sqlite3_column_text(statement, 4), 64);	//256 bit hash to 32 bytes hash to 64 byte hex text

		media_list->push_back(tmp);
	});
}

int 
MediaDatabase::InsertMedia(const MediaInfo& new_media) {

	//replace all single quote(') in media name to double single quote ('') to escape from sql query
	const QString clean_sub_path = QString(new_media.sub_path).replace('\'', "''");
	const QString clean_long_name = QString(new_media.long_name).replace('\'', "''");
	const QString clean_short_name = QString(new_media.short_name).replace('\'', "''");

	const QString query = "INSERT INTO MEDIA (sub_path, name, alt_name, hash) VALUES('" %
							clean_sub_path % "','" %
							clean_long_name % "','" %
							clean_short_name % "','" %
							new_media.hash % "');";

	return SingleStepQuery(query);
}

int	
MediaDatabase::InsertMediaList(const QVector<MediaInfo>& new_media_list) {

	const QString transaction_format_str = "INSERT INTO MEDIA (sub_path, name, alt_name, hash) VALUES('%1', '%2', '%3', '%4');";
	QString query = "BEGIN TRANSACTION;";


	QString clean_sub_path, clean_long_name, clean_short_name;

	for (auto iter = new_media_list.begin(); iter < new_media_list.end(); iter++) {

		//replace all single quote(') in media name to double single quote ('') to escape from sql query

		clean_sub_path = QString(iter->sub_path).replace('\'', "''");
		clean_long_name = QString(iter->long_name).replace('\'', "''");
		clean_short_name = QString(iter->short_name).replace('\'', "''");
		
		query.append(transaction_format_str.arg(clean_sub_path, clean_long_name, clean_short_name, iter->hash));
	}

	query.append("COMMIT;");

	return SingleStepMultiStatementQuery(query);
}

int
MediaDatabase::UpdateMedia(const MediaInfo& media) {
	
	//replace all single quote(') in media name to double single quote ('') to escape from sql query
	const QString clean_sub_path = QString(media.sub_path).replace('\'', "''");
	const QString clean_long_name = QString(media.long_name).replace('\'', "''");
	const QString clean_short_name = QString(media.short_name).replace('\'', "''");

	const QString query =		"UPDATE MEDIA SET sub_path = '" % clean_sub_path % "'," %
								"name = '" % clean_long_name % "'," %
								"alt_name = '" % clean_short_name % "'," %
								"hash = '" % media.hash % "' WHERE id = " % QString::number(media.id) % ";";
	
	return SingleStepQuery(query);
}

int 
MediaDatabase::UpdateMediaList(const QVector<MediaInfo>& media_list) {
	const QString transaction_format_str = "UPDATE MEDIA SET sub_path = '%1', name = '%2', alt_name = '%3', hash = '%4' WHERE id = %5;";
	QString query = "BEGIN TRANSACTION;";

	QString clean_sub_path, clean_long_name, clean_short_name;

	for (auto iter = media_list.begin(); iter < media_list.end(); iter++) {
		//replace all single quote(') in media name to double single quote ('') to escape from sql query

		clean_sub_path = QString(iter->sub_path).replace('\'', "''");
		clean_long_name = QString(iter->long_name).replace('\'', "''");
		clean_short_name = QString(iter->short_name).replace('\'', "''");

		query.append(transaction_format_str.arg(clean_sub_path, clean_long_name, clean_short_name, iter->hash, QString::number(iter->id)));
	}

	query.append("COMMIT;");

	return SingleStepMultiStatementQuery(query);
}

int
MediaDatabase::RemoveMedia(const unsigned int media_id) {
	const QString query = "DELETE FROM MEDIA WHERE id = " % QString::number(media_id) % ';';
	return SingleStepQuery(query);
}

int 
MediaDatabase::RemoveMediaList(const QVector<unsigned int>& media_id_list) {
	const QString transaction_format_str = "DELETE FROM MEDIA WHERE id = %1;";
	QString query = "BEGIN TRANSACTION;";


	for (auto iter = media_id_list.begin(); iter < media_id_list.end(); iter++) {
		query.append(transaction_format_str.arg(QString::number(*iter)));
	}

	query.append("COMMIT;");

	return SingleStepMultiStatementQuery(query);
}