#include "pch.h"

#include <QStringBuilder>
#include <QTextStream>
#include <QStack>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QFuture>
#include <QtConcurrent>
#include <iostream>
#include <unordered_set>
#include <algorithm>


#include "daemon.h"
#include "config.h"
#include "util.h"
#include "query.h"
#include "error.h"

Daemon::Daemon() 
{
}

Daemon::~Daemon() {
	//post event to break from monitor loop
	Stop();

	//TODO: if file tracker has outstanding soft remove file, remove it now

	tag_db.Close();
	tag_link_db.Close();
	media_db.Close();

	//wait to run() to finish
	wait();
}

void
Daemon::Stop() {
	//tell termination event to break out of monitor loop
	SetEvent(monitor_terminate_event);
}

QString
Daemon::GetRootDirectory() {
	return abs_root_dir;
}


int
Daemon::AddTag(const QString& name, unsigned int* new_tag_id /*= nullptr*/) {

	tag_list_lock.lockForWrite();

	//check if tag with name already exist
	if (global_tag_list.TagExistByName(name)) {
		tag_list_lock.unlock();
		Logger::Log("Tag name: " % name % " already exist", LogEntry::LT_ERROR);
		return -1;
	}


	unsigned int id;
	global_tag_list.InsertNewTag(name, &id);

	if (new_tag_id != nullptr) {
		*new_tag_id = id;
	}
	
	Tag tmp;
	global_tag_list.GetTagById(id, &tmp);
	
	ModelTag buff;
	buff.id = tmp.id;
	buff.name = tmp.name;
	buff.media_count = tmp.media_id_list.size();

	tag_list_lock.unlock();

	if(tag_db.InsertTag(tmp) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}
	
	emit TagInserted(buff);

	Logger::Log("Tag: " % buff.name % " added", LogEntry::LT_SUCCESS);
	return 1;
}

int
Daemon::UpdateTagName(const unsigned int tag_id, const QString& new_name) {

	tag_list_lock.lockForWrite();

	//check if new name already exist
	if (global_tag_list.TagExistByName(new_name)) {
		tag_list_lock.unlock();
		Logger::Log("Tag name: " % new_name % " already exist", LogEntry::LT_ERROR);
		return -1;
	}

	//check if tag_id exist
	if (!global_tag_list.TagExistById(tag_id)) {
		tag_list_lock.unlock();

		Logger::Log("Tag id: " % QString::number(tag_id) % " does not exist", LogEntry::LT_ERROR);
		return -2;
	}

	global_tag_list.UpdateTagName(tag_id, new_name);

	tag_list_lock.unlock();

	Tag buff;
	global_tag_list.GetTagById(tag_id, &buff);

	if (tag_db.UpdateTag(buff) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	ModelTag m_tag;
	buff.FormModelTag(&m_tag);
	emit TagNameUpdated(m_tag.id, new_name);

	Logger::Log("Tag id: " % QString::number(tag_id) % " name updated to: " % buff.name, LogEntry::LT_SUCCESS);

	return 1;
}

int
Daemon::RemoveTagById(const unsigned int tag_id) {
	
	int ret;

	Tag tag;

	tag_list_lock.lockForWrite();

	//check if tag_id exist
	if (!global_tag_list.TagExistById(tag_id)) {
		tag_list_lock.unlock();

		Logger::Log("Tag id: " % QString::number(tag_id) % " does not exist", LogEntry::LT_ERROR);
		return -1;
	}

	global_tag_list.GetTagById(tag_id, &tag);

	media_list_lock.lockForWrite();

	//remove reference to this tag id from all media linked with this tag
	for (auto iter = tag.media_id_list.begin(); iter != tag.media_id_list.end(); iter++) {
		
		global_media_list.RemoveMediaTag(tag_id, *iter);

		//if the media becomes tagless inform gui 
		if (global_media_list.GetMediaTagCount(*iter) == 0) {

			Media media;
			
			global_media_list.GetMediaById(*iter, &media);

			emit TaglessMediaInserted(media.FormModelMedia(abs_root_dir));
		}
	}

	//remove this tag from memory
	global_tag_list.RemoveTagById(tag_id);

	tag_list_lock.unlock();
	media_list_lock.unlock();

	//remove this tag from link db
	if (tag_link_db.RemoveTagLinkByTagId(tag_id) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	//remove this tag from tag db
	if (tag_db.RemoveTag(tag_id) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	emit TagRemoved(tag_id);

	Logger::Log("Tag id: " % QString::number(tag_id) % " name: " % tag.name % " removed", LogEntry::LT_SUCCESS);
	return 1;
}

int
Daemon::FormLinkByTagName(const QString& tag_name, const unsigned int media_id) {

	tag_list_lock.lockForWrite();
	media_list_lock.lockForWrite();

	//check if tag exist
	if (!global_tag_list.TagExistByName(tag_name)) {
		tag_list_lock.unlock();
		media_list_lock.unlock();

		Logger::Log("Tag name: " % tag_name % " does not exist", LogEntry::LT_ERROR);
		return -1;
	}

	unsigned int tag_id;
	global_tag_list.GetTagIdByName(tag_name, &tag_id);

	//check if media exist
	if (!global_media_list.MediaExistById(media_id)) {
		tag_list_lock.unlock();
		media_list_lock.unlock();
		return -2;
	}

	//check media doesnt already have this tag
	if (global_media_list.MediaTagIdExist(media_id, tag_id)) {
		tag_list_lock.unlock();
		media_list_lock.unlock();

		Logger::Log("Tag name: " % tag_name % " is already linked with media id: " % QString::number(media_id), LogEntry::LT_ERROR);
		return -3;
	}
	
	return FormLink(tag_id, media_id);
}

int
Daemon::FormLinkByIds(const unsigned int tag_id, const unsigned int media_id) {

	tag_list_lock.lockForWrite();
	media_list_lock.lockForWrite();

	return FormLink(tag_id, media_id);
}

int 
Daemon::FormMediaMappedLink(const QVector<MediaInfo>& media_list) {
	QVector<unsigned int> media_ids;
	QVector<unsigned int> tag_ids;

	QVector<QString> tag_name_list;

	tag_list_lock.lockForWrite();
	
	Tag tag_buff;
	ModelTag m_tag_buff;

	for (auto media_iter = media_list.cbegin(); media_iter != media_list.cend(); media_iter++) {

		tag_name_list.clear();
		mediamap.GetMappedTags(media_iter->GetSubpathLongName(), &tag_name_list);
		
		if (tag_name_list.empty()) {
			continue;
		}

		Logger::Log("Applying filename mapped link for: " % media_iter->long_name);

		for (auto tag_name_iter = tag_name_list.cbegin(); tag_name_iter != tag_name_list.cend(); tag_name_iter++) {

			if (!global_tag_list.TagExistByName(*tag_name_iter)) {

				Logger::Log("Tag name: " % *tag_name_iter % " does not exist", LogEntry::LT_ERROR);
				continue;
			}

			global_tag_list.GetTagByName(*tag_name_iter, &tag_buff);
				

			media_list_lock.lockForWrite();

			global_media_list.InsertMediaTag(tag_buff.id, media_iter->id);
			global_tag_list.InsertTagMedia(tag_buff.id, media_iter->id);

			media_ids.push_back(media_iter->id);
			tag_ids.push_back(tag_buff.id);

			media_list_lock.unlock();

			tag_buff.FormModelTag(&m_tag_buff);

			emit LinkFormed(m_tag_buff, media_iter->id);
			Logger::Log("Linked formed: Tag: " % m_tag_buff.name % " media id: " % QString::number(media_iter->id), LogEntry::LT_SUCCESS);
			
		}
	}

	//update to database
	if (tag_link_db.CreateTagLinkByTagMediaIdList(tag_ids, media_ids) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		tag_list_lock.unlock();
		return -Error::DAEMON_DB;
	}

	tag_list_lock.unlock();
	return 1;
}

int 
Daemon::DestroyLink(const unsigned int tag_id, const unsigned int media_id) {

	//destroy in memory

	tag_list_lock.lockForWrite();
	media_list_lock.lockForWrite();

	global_tag_list.RemoveTagMedia(tag_id, media_id);
	global_media_list.RemoveMediaTag(tag_id, media_id);

	//destroy in database
	if (tag_link_db.RemoveTagLinkByTagIdMediaId(tag_id, media_id) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		tag_list_lock.unlock();
		media_list_lock.unlock();
		return -Error::DAEMON_DB;
	}

	tag_list_lock.unlock();
	media_list_lock.unlock();

	emit LinkDestroyed(tag_id, media_id);

	if (global_media_list.GetMediaTagCount(media_id) == 0) {

		Media tmp;

		global_media_list.GetMediaById(media_id, &tmp);
		
		emit TaglessMediaInserted(tmp.FormModelMedia(abs_root_dir));
	}

	Logger::Log("Link destroyed between tag id: " % QString::number(tag_id) % " and media id: " % QString::number(media_id), LogEntry::LT_SUCCESS);
	return 1;
}

int 
Daemon::AddMedia(const QString& sub_path, const QString& name, const QString& alt_name, const QString& hash /*optional*/) {

	MediaInfo new_media(sub_path, name, alt_name, hash);

	if (hash.isEmpty()) {
		//compute hash
		if (FileUtil::GetFileSHA2(abs_root_dir, new_media, &new_media.hash) < 0) {
			Logger::Log("Failed to calculate hash for file: " % new_media.long_name, LogEntry::LT_WARNING);
			new_media.hash.clear();
		}
	}


	if (media_db.InsertMedia(new_media) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	new_media.id = media_db.LastRowId();
	//a 0 as row id actually means no row was successfully inserted which is an error
	if (new_media.id == 0) {
		Logger::Log(QString(DAEMON_DB_MSG) % ": row id returned 0", LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	media_list_lock.lockForWrite();

	global_media_list.InsertMedia(new_media);

	media_list_lock.unlock();

	ModelMedia buff = new_media.FormModelMedia(abs_root_dir);

	emit TaglessMediaInserted(buff);

	file_tracker.AddMediaSubPath(sub_path, new_media.id);

	Logger::Log("Media: " % buff.name % " id: " % QString::number(buff.id) % " added", LogEntry::LT_SUCCESS);
	//return new media id
	return new_media.id;
}

int 
Daemon::AddMediaList(QVector<MediaInfo>& media_list) {

	/*
	//compute hash func
	auto hash_map_func = [this](MediaInfo& media) -> void {

		if (FileUtil::GetFileSHA2(this->abs_root_dir, media) < 0) {
			Logger::Log("Failed to calculate hash for file: " % media.long_name, LogEntry::LT_WARNING);
			media.hash.clear();
		}
	};

	QtConcurrent::blockingMap(media_list, hash_map_func);
	*/

	//bulk insert db
	if (media_db.InsertMediaList(media_list) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	//get id from db
	unsigned int item_id = media_db.LastRowId();
	for (auto iter = media_list.rbegin(); iter != media_list.rend(); iter++) {
		iter->id = item_id;
		item_id--;
	}

	for (auto iter = media_list.begin(); iter != media_list.end(); iter++) {
		//insert into global media list 
		media_list_lock.lockForWrite();

		global_media_list.InsertMedia(*iter);

		media_list_lock.unlock();

		//insert into file tracker
		file_tracker.AddMediaSubPath(iter->sub_path, iter->id);

		//emit new tagless media
		ModelMedia buff = iter->FormModelMedia(abs_root_dir);
		emit TaglessMediaInserted(buff);
		Logger::Log("Media: " % buff.name % " id: " % QString::number(buff.id) % " added", LogEntry::LT_SUCCESS);
	}

	return 1;
}

int 
Daemon::UpdateMediaName(const unsigned int media_id, const QString& long_name, const QString& short_name) {
	
	MediaInfo tmp;
	
	media_list_lock.lockForWrite();

	global_media_list.UpdateMediaName(media_id, long_name, short_name);
	global_media_list.GetMediaInfoById(media_id, &tmp);

	media_list_lock.unlock();

	emit MediaNameUpdated(media_id, long_name);

	if (media_db.UpdateMedia(tmp) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	Logger::Log("Media id: " % QString::number(media_id) % " name update to: " % long_name, LogEntry::LT_SUCCESS);

	return 1;
}

int 
Daemon::UpdateMediaSubdir(const unsigned int media_id, const QString& sub_dir) {

	MediaInfo tmp;

	media_list_lock.lockForWrite();

	global_media_list.UpdateMediaSubdir(media_id, sub_dir);
	global_media_list.GetMediaInfoById(media_id, &tmp);

	media_list_lock.unlock();

	QString abs_path = abs_root_dir;

	if (tmp.sub_path.size() > 0) {
		abs_path.append(tmp.sub_path);
	}

	abs_path.append('\\');

	abs_path.append(tmp.long_name);

	emit MediaSubdirUpdated(media_id, abs_path);

	if (media_db.UpdateMedia(tmp) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	Logger::Log("Media id: " % QString::number(media_id) % " subdir updated to: " % sub_dir, LogEntry::LT_SUCCESS);
	return 1;
}

int 
Daemon::UpdateMediaListSubdir(const QVector<unsigned int>& media_id_list, const QVector<QString>& new_sub_dir) {

	QVector<MediaInfo> media_list;
	media_list.reserve(media_id_list.size());

	MediaInfo tmp_media;
	QString new_abs_path;

	auto sub_dir_iter = new_sub_dir.begin();	//sub dir list is guaranteed in sync with media id  and same size
	for (auto id_iter = media_id_list.begin(); id_iter < media_id_list.end(); id_iter++) {

		media_list_lock.lockForWrite();

		global_media_list.UpdateMediaSubdir(*id_iter, *sub_dir_iter);
		global_media_list.GetMediaInfoById(*id_iter, &tmp_media);

		media_list_lock.unlock();

		media_list.push_back(tmp_media);

		new_abs_path = abs_root_dir;

		if (tmp_media.sub_path.size() > 0) {
			new_abs_path.append(tmp_media.sub_path);
		}

		new_abs_path.append('\\');

		new_abs_path.append(tmp_media.long_name);

		emit MediaSubdirUpdated(*id_iter, new_abs_path);

		Logger::Log("Media id: " % QString::number(*id_iter) % " subdir updated to: " % *sub_dir_iter, LogEntry::LT_SUCCESS);

		sub_dir_iter++;
	}

	if (media_db.UpdateMediaList(media_list) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	return 1;
}

int
Daemon::RemoveMedia(const unsigned int media_id) {

	Media media;

	tag_list_lock.lockForWrite();
	media_list_lock.lockForWrite();

	global_media_list.GetMediaById(media_id, &media);

	//remove media ptr from tags that are linked with this media
	for (auto iter = media.tag_id_list.begin(); iter != media.tag_id_list.end(); iter++) {
		global_tag_list.RemoveTagMedia(*iter, media_id);

		emit LinkDestroyed(*iter, media_id);
	}

	tag_list_lock.unlock();

	//remove all links in db with this media
	if (tag_link_db.RemoveTagLinkByMediaId(media_id) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	//remove this media from db
	if (media_db.RemoveMedia(media_id) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	//remove this media from memory
	global_media_list.RemoveMedia(media_id);

	media_list_lock.unlock();

	if (file_tracker.DirExist(media.sub_path)) {
		file_tracker.RemoveMedia(media.sub_path, media_id);
	}

	emit MediaRemoved(media_id);

	Logger::Log("Media id: " % QString::number(media_id) % " removed", LogEntry::LT_SUCCESS);

	return 1;
}

int 
Daemon::RemoveMediaList(const QVector<unsigned int>& media_id_list) {
	
	Media media;

	for (auto media_id_iter = media_id_list.begin(); media_id_iter != media_id_list.end(); media_id_iter++) {
		
		tag_list_lock.lockForWrite();
		media_list_lock.lockForWrite();

		global_media_list.GetMediaById(*media_id_iter, &media);

		//remove media ptr from tags that are linked with this media
		for (auto tag_id_iter = media.tag_id_list.begin(); tag_id_iter != media.tag_id_list.end(); tag_id_iter++) {
			global_tag_list.RemoveTagMedia(*tag_id_iter, *media_id_iter);

			emit LinkDestroyed(*tag_id_iter, *media_id_iter);
		}

		tag_list_lock.unlock();

		//remove this media from memory
		global_media_list.RemoveMedia(*media_id_iter);

		media_list_lock.unlock();

		if (file_tracker.DirExist(media.sub_path)) {
			file_tracker.RemoveMedia(media.sub_path, *media_id_iter);
		}

		emit MediaRemoved(*media_id_iter);

		Logger::Log("Media id: " % QString::number(*media_id_iter) % " removed", LogEntry::LT_SUCCESS);
	}

	if (tag_link_db.RemoveTagLinkByMediaIdList(media_id_list) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	if (media_db.RemoveMediaList(media_id_list) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		return -Error::DAEMON_DB;
	}

	return 1;
}

int 
Daemon::AddDir(const QString& sub_path, const QString& long_name, const QString& short_name) {

	Logger::Log("New Dir: " % long_name % " added", LogEntry::LT_SUCCESS);
	return file_tracker.AddDirSubPath(sub_path, long_name, short_name);
}

int 
Daemon::UpdateDirName(const QString& sub_path_name, const QString& long_name, const QString& short_name) {

	QVector<unsigned int> affected_media_id_list;
	
	file_tracker.GetDirMediaIdRecurByPath(sub_path_name, &affected_media_id_list);
	file_tracker.UpdateDirName(sub_path_name, long_name, short_name);

	Logger::Log("Directory: " % sub_path_name % " name update to: " % long_name, LogEntry::LT_SUCCESS);

	if (affected_media_id_list.empty()) {
		return 1;
	}

	QString dir_sub_path = sub_path_name.chopped(sub_path_name.size() - sub_path_name.lastIndexOf('\\'));
	
	//update name for all media
	
	MediaInfo info_buff;
	QVector<QString>		new_subdir_list;

	int next_slash_idx;
	int replace_len;

	for (auto iter = affected_media_id_list.begin(); iter != affected_media_id_list.end(); iter++) {
		
		media_list_lock.lockForRead();

		global_media_list.GetMediaInfoById(*iter, &info_buff);

		media_list_lock.unlock();

		next_slash_idx = info_buff.sub_path.indexOf('\\', dir_sub_path.size() + 1);
		if (next_slash_idx == -1) {
			replace_len = info_buff.sub_path.size() - dir_sub_path.size() - 1;
		}
		else {
			replace_len = next_slash_idx - dir_sub_path.size() - 1;
		}

		info_buff.sub_path.replace(dir_sub_path.size() + 1, replace_len, long_name);

		new_subdir_list.push_back(info_buff.sub_path);
	}

	if (affected_media_id_list.size() == 1) {
		UpdateMediaSubdir(affected_media_id_list[0], new_subdir_list[0]);
	}
	else {
		UpdateMediaListSubdir(affected_media_id_list, new_subdir_list);
	}

	return 1;
}

int 
Daemon::UpdateDirSubDir(const QString& sub_path_name, const QString& new_sub_path) {

	QVector<unsigned int> affected_media_id_list;

	file_tracker.GetDirMediaIdRecurByPath(sub_path_name, &affected_media_id_list);
	file_tracker.UpdateDirSubdir(sub_path_name, new_sub_path);

	Logger::Log("Directory: " % sub_path_name % " subdir updated to: " % new_sub_path, LogEntry::LT_SUCCESS);

	if (affected_media_id_list.empty()) {
		return 1;
	}

	QString old_dir_sub_path = sub_path_name.chopped(sub_path_name.size() - sub_path_name.lastIndexOf('\\'));

	//update name for all media

	MediaInfo info_buff;

	QVector<QString>		new_subdir_list;

	for (auto iter = affected_media_id_list.begin(); iter != affected_media_id_list.end(); iter++) {

		media_list_lock.lockForRead();

		global_media_list.GetMediaInfoById(*iter, &info_buff);

		media_list_lock.unlock();

		info_buff.sub_path.replace(0, old_dir_sub_path.size(), new_sub_path);

		//UpdateMediaSubdir(*iter, info_buff.sub_path);
		new_subdir_list.push_back(info_buff.sub_path);
	}

	if (affected_media_id_list.size() == 1) {
		UpdateMediaSubdir(affected_media_id_list[0], new_subdir_list[0]);
	}
	else {
		UpdateMediaListSubdir(affected_media_id_list, new_subdir_list);
	}

	return 1;
}

int 
Daemon::RemoveDir(const QString& sub_path_name) {
	QVector<unsigned int> affected_media_id_list;

	file_tracker.GetDirMediaIdRecurByPath(sub_path_name, &affected_media_id_list);

	if (affected_media_id_list.size() == 1) {
		RemoveMedia(affected_media_id_list[0]);
	}
	else if (affected_media_id_list.size() > 1) {
		RemoveMediaList(affected_media_id_list);
	}

	file_tracker.RemoveDir(sub_path_name);

	Logger::Log("Directory: " % sub_path_name % " removed", LogEntry::LT_SUCCESS);
	return 1;
}

MediaModelResult
Daemon::GetAllMedia() {

	
	QVector<Media*> tmp_media_vec;
	
	media_list_lock.lockForRead();

	global_media_list.GetAllMediaPtr(&tmp_media_vec);

	MediaModelResult result;

	result.model_media_list.reserve(tmp_media_vec.size());

	for (auto iter = tmp_media_vec.begin(); iter != tmp_media_vec.end(); iter++) {

		result.model_media_list.push_back((*iter)->FormModelMedia(abs_root_dir) );
	}

	media_list_lock.unlock();

	return result;
}

MediaModelResult
Daemon::GetAllTaglessMedia() {
	QVector<Media*> tmp_media_vec;

	media_list_lock.lockForRead();

	global_media_list.GetAllMediaPtr(&tmp_media_vec);

	MediaModelResult result;
	result.model_media_list.reserve(tmp_media_vec.size());

	for (auto iter = tmp_media_vec.begin(); iter != tmp_media_vec.end(); iter++) {

		if (!(*iter)->tag_id_list.empty()) {
			continue;
		}

		result.model_media_list.push_back((*iter)->FormModelMedia(abs_root_dir));
	}

	media_list_lock.unlock();

	return result;
}

MediaModelResult
Daemon::GetTagMedias(const unsigned int tag_id) {
	MediaModelResult result;

	Tag tag_buff;

	tag_list_lock.lockForRead();

	if (!global_tag_list.TagExistById(tag_id)) {
		tag_list_lock.unlock();

		return result;
	}

	media_list_lock.lockForRead();

	global_tag_list.GetTagById(tag_id, &tag_buff);

	result.model_media_list.reserve(tag_buff.media_id_list.size());

	Media media_buff;

	for (auto iter = tag_buff.media_id_list.begin(); iter != tag_buff.media_id_list.end(); iter++) {

		global_media_list.GetMediaById(*iter, &media_buff);
		
		result.model_media_list.push_back(media_buff.FormModelMedia(abs_root_dir));
	}

	tag_list_lock.unlock();
	media_list_lock.unlock();

	result.associated_tag_id_set.insert(tag_id);

	return result;
}

MediaModelResult
Daemon::GetQueryMedia(const QString& raw_query_str) {

	MediaModelResult result;
	Query query(raw_query_str);


	auto get_tag_media_list_handler = [this, &result](const QString& tag_name, QSet<unsigned int> *out) {

		Tag tag_buff;

		this->tag_list_lock.lockForRead();

		if (!this->global_tag_list.TagExistByName(tag_name)) {
			Logger::Log("Tag name: " % tag_name % " doesn't exist", LogEntry::LT_ERROR);
			this->tag_list_lock.unlock();
			return -1;
		}

		this->global_tag_list.GetTagByName(tag_name, &tag_buff);
		this->tag_list_lock.unlock();

		*out = std::move(tag_buff.media_id_list);
		result.associated_tag_id_set.insert(tag_buff.id);

		return 1;
	};



	if (query.Tokenize(get_tag_media_list_handler) < 0) {
		Logger::Log("Failed tokenizing query", LogEntry::LT_ERROR);
		result.associated_tag_id_set.clear();
		return result;
	}

	/*
	QVector<QString> tag_name_list;
	query.GetTagNameList(&tag_name_list);

	//fill query will tag media ids 
	Tag tag_buff;
	for (QString tag_name : tag_name_list) {

		tag_list_lock.lockForRead();

		if (!global_tag_list.TagExistByName(tag_name)) {
			Logger::Log("Tag name: " % tag_name % " doesn't exist", LogEntry::LT_ERROR);
			result.associated_tag_id_set.clear();
			tag_list_lock.unlock();
			return result;
		}

		global_tag_list.GetTagByName(tag_name, &tag_buff);
		tag_list_lock.unlock();

		query.InsertTagMediaIds(tag_name, tag_buff.media_id_list);
		result.associated_tag_id_set.insert(tag_buff.id);
	}*/

	if (query.GenerateAST() < 0) {
		Logger::Log("Failed generating query AST", LogEntry::LT_ERROR);
		result.associated_tag_id_set.clear();
		return result;
	}

	if (query.ProcessAST() < 0) {
		Logger::Log("Failed processing query AST", LogEntry::LT_ERROR);
		result.associated_tag_id_set.clear();
		return result;
	}

	media_list_lock.lockForRead();

	Media media_buff;
	for (unsigned int media_id : query.result) {
		if(global_media_list.MediaExistById(media_id)) {
			global_media_list.GetMediaById(media_id, &media_buff);
			
			result.model_media_list.push_back(media_buff.FormModelMedia(abs_root_dir));
		}
	}
	media_list_lock.unlock();

	Logger::Log("Query processed", LogEntry::LT_SUCCESS);
	return result;
}

QVector<ModelTag>
Daemon::GetMediaTags(const unsigned int media_id) {
	QVector<ModelTag> ret_vec;
	QVector<unsigned int> tag_ids;

	tag_list_lock.lockForRead();
	media_list_lock.lockForRead();

	if (!global_media_list.MediaExistById(media_id)) {
		tag_list_lock.unlock();
		media_list_lock.unlock();

		return ret_vec;
	}

	global_media_list.GetMediaTagIds(media_id, &tag_ids);

	ret_vec.reserve(tag_ids.size());

	ModelTag m_tag_buff;
	Tag tag_buff;
	for (auto iter = tag_ids.begin(); iter != tag_ids.end(); iter++) {
		global_tag_list.GetTagById(*iter, &tag_buff);

		tag_buff.FormModelTag(&m_tag_buff);

		ret_vec.push_back(m_tag_buff);
	}

	tag_list_lock.unlock();
	media_list_lock.unlock();

	return ret_vec;
}

QVector<ModelTag>	
Daemon::GetAllTags() {
	QVector<ModelTag> ret_vec;

	QVector<Tag> tag_vec;

	tag_list_lock.lockForRead();

	global_tag_list.GetAllTags(&tag_vec);

	ModelTag m_tag_buff;
	for (auto iter = tag_vec.begin(); iter != tag_vec.end(); iter++) {
		iter->FormModelTag(&m_tag_buff);
		ret_vec.push_back(m_tag_buff);
	}

	tag_list_lock.unlock();

	return ret_vec;
}

//protected =========================
void
Daemon::run() {

	Init();

	//enters monitoring loop - calls ReadDirectoryChangesW and process results
	Notify notifier(abs_root_dir.toStdWString());
	notifier.InitHandle();

	NotifyEvent notify_event;

	for (;;) {

		//immediate send an io request then tries to do somework 
		notifier.RequestChanges();

		//see if there's events to be processed
		while (notifier.HasEvent()) {
			notifier.GetNextEvent(&notify_event);

			//skip this event because it has been processed fron previous iteration
			if (notify_event.event == NotifyEvent::RENAME_SKIP) {
				Logger::Log("Evt: RENAME NEW: " % QString::fromStdWString(notify_event.file_wname) % "\tPath: " % QString::fromStdWString(notify_event.dir_wname), LogEntry::LT_MONITOR);
				continue;
			}

			ProcessNotifyEvent(notify_event, notifier);
		}


		if (file_tracker.soft_del_flag) {

			//put to alertable state wait

			//give 1 second until next events come
			int wait_ret = WaitForSingleObjectEx(monitor_terminate_event, 1000, true);
			
			if (wait_ret == WAIT_OBJECT_0) {
				
				//wait disturbed by terminate event being signaled
				if (file_tracker.soft_del_dir_flag) {
					RemoveDir(file_tracker.soft_del_dir_sub_path_name);
				}
				else {
					RemoveMedia(file_tracker.soft_del_media.id);
				}
				
				file_tracker.soft_del_flag = false;
				break;

			}
			else if (wait_ret == WAIT_IO_COMPLETION) {
				//receieved another event
				continue;
			}
			else if (wait_ret == WAIT_TIMEOUT) {
				
				// no events within the waited period - hard delete
				if (file_tracker.soft_del_dir_flag) {
					RemoveDir(file_tracker.soft_del_dir_sub_path_name);
				}
				else {
					RemoveMedia(file_tracker.soft_del_media.id);
				}
				file_tracker.soft_del_flag = false;
			}
			
		}

		//put to alertable state wait
		int wait_ret = WaitForSingleObjectEx(monitor_terminate_event, INFINITE, true);
		//wait disturbed by terminate event being signaled
		if (wait_ret == WAIT_OBJECT_0) {
			break;
		}
		
		//wait disturbed by completion routine here
	}

	//clean up after thread finishes
	CloseHandle(monitor_terminate_event);

	Logger::Log("Daemon thread ends", LogEntry::LT_SUCCESS);
}

//private =========================
void
Daemon::Init() {
	int ret;

	Logger::Log("Initializing daemon...", LogEntry::LT_ATTN);

	{
		QDir curr_dir = QDir::current();
		working_subdir = curr_dir.dirName();

		Logger::Log("Working subdir: " % working_subdir);

		if (!curr_dir.cdUp()) {
			Logger::Log("Invalid root dir", LogEntry::LT_ERROR);
			return;
		}

		abs_root_dir = QDir::toNativeSeparators(curr_dir.absolutePath());
		Logger::Log("Root tracking absolute dir: " % abs_root_dir);

		file_tracker.SetRootDir(abs_root_dir);
	}

	Logger::Log("Loading ignorefile...", LogEntry::LT_ATTN);
	LoadIgnoreList();

	Logger::Log("Initializing filename map...", LogEntry::LT_ATTN);
	LoadFilenameToTagMap();

	
	Logger::Log("Initalizing databases...", LogEntry::LT_ATTN);
	ret = InitDB();
	if (ret < 0) {
		return;
	}

	QString db_path_buff = tag_db.GetPath();

	Logger::Log("Loading from tag database " % db_path_buff % "...");

	ret = tag_db.GetAllTags(&global_tag_list);
	if (ret < 0) {
		return;
	}
	Logger::Log(QString::number(global_tag_list.GetSize()) % " tags loaded", LogEntry::LT_SUCCESS);

	db_path_buff = media_db.GetPath();
	Logger::Log("Loading from media database " % db_path_buff % "...");
	QVector<MediaInfo> db_media_vector;
	ret = media_db.GetAllMedia(&db_media_vector);
	if (ret < 0) {
		return;
	}
	Logger::Log(QString::number(db_media_vector.size()) % " media loaded", LogEntry::LT_SUCCESS);

	Logger::Log("Validating loaded media...", LogEntry::LT_ATTN);
	QVector<MediaInfo> soft_delete_media_vec;
	ValidateMedia(db_media_vector, &soft_delete_media_vec);

	Logger::Log("Scanning for new media...", LogEntry::LT_ATTN);
	QVector<MediaInfo> new_media_list;
	DiscoverNewMedia(new_media_list);

	if (!soft_delete_media_vec.empty()) {
		Logger::Log("Resolving soft deleted media...", LogEntry::LT_ATTN);
		ResolveNewAndSoftDeletedMedia(soft_delete_media_vec, new_media_list);
	}

	Logger::Log(QString::number(new_media_list.size()) % " new media found");

	if (!new_media_list.empty()) {

		if (new_media_list.size() > 1) {
			AddMediaList(new_media_list);
		}
		else if (new_media_list.size() == 1) {
			new_media_list[0].id = AddMedia(new_media_list[0].sub_path,
				new_media_list[0].long_name,
				new_media_list[0].short_name,
				new_media_list[0].hash);
		}

		FormMediaMappedLink(new_media_list);
	}

	{
		QVector<TagLink> tag_link_vec;

		db_path_buff = tag_link_db.GetPath();
		Logger::Log("Loading from tag link database " % db_path_buff % "...");
		ret = tag_link_db.GetAllTagLinks(&tag_link_vec);
		if (ret < 0) {
			return;
		}
		Logger::Log(QString::number(tag_link_vec.size()) % " tag links loaded", LogEntry::LT_SUCCESS);

		Logger::Log("Resolving tag links...", LogEntry::LT_ATTN);
		ResolveTagLinks(tag_link_vec);
	}

	Logger::Log("Populating file tracker directory medid id...", LogEntry::LT_ATTN);
	PopulateDirMediaId();

	Logger::Log("Creating momnitor control event", LogEntry::LT_ATTN);
	monitor_terminate_event = CreateEvent(NULL, true, false, L"MonitorTermEvt");
	if (monitor_terminate_event == NULL) {
		Logger::Log("Failed to create monitor termination event", LogEntry::LT_ERROR);
		return;
	}

	emit Initialized();

	Logger::Log("Daemon intialized", LogEntry::LT_SUCCESS);
}

int
Daemon::InitDB() {

	int ret;

	QString curr_path = QDir::fromNativeSeparators(QDir::currentPath());

	curr_path.push_back('\\');

	tag_db.SetPath(curr_path + TAGS_DB_NAME);
	tag_link_db.SetPath(curr_path + TAGLINK_DB_NAME);
	media_db.SetPath(curr_path + MEDIA_DB_NAME);

	//initalize

	if (tag_db.Init() < 0) {
		Logger::Log(QString(DAEMON_INIT_DB_MSG) % ": tag database", LogEntry::LT_ERROR);
		return -Error::DAEMON_INIT_DB;
	}

	if (tag_link_db.Init() < 0) {
		Logger::Log(QString(DAEMON_INIT_DB_MSG) % ": tag link database", LogEntry::LT_ERROR);
		return -Error::DAEMON_INIT_DB;
	}

	if (media_db.Init() < 0) {
		Logger::Log(QString(DAEMON_INIT_DB_MSG) % ": media database", LogEntry::LT_ERROR);
		return -Error::DAEMON_INIT_DB;
	}

	Logger::Log("Databases initialized", LogEntry::LT_SUCCESS);

	return 1;
}

int
Daemon::ResolveTagLinks(const QVector<TagLink>& tag_link_vec) {

	int ret;
	Media media;

	QSet<unsigned int> link_delete_list_by_tag_id;
	QSet<unsigned int> link_delete_list_by_media_id;

	for (auto iter = tag_link_vec.cbegin(); iter != tag_link_vec.cend(); iter++) {

		if (!global_tag_list.TagExistById(iter->tag_id)) {
			//tag doesn't exist
			//this could only happen when user manually deletes a row of tag from tags.sqlite3 outside of program execution
			
			//insert this tag id to be deleted
			if (link_delete_list_by_tag_id.find(iter->tag_id) == link_delete_list_by_tag_id.end()) {
				link_delete_list_by_tag_id.insert(iter->tag_id);
			}
			continue;
		}

		if (!global_media_list.MediaExistById(iter->media_id)) {
			//media doesn't exist
			//insert this media id to be deleted
			if (link_delete_list_by_media_id.find(iter->media_id) == link_delete_list_by_media_id.end()) {
				link_delete_list_by_media_id.insert(iter->media_id);
			}
			continue;
		}


		global_media_list.GetMediaById(iter->media_id, &media);

		global_tag_list.InsertTagMedia(iter->tag_id, media.id);
		global_media_list.InsertMediaTag(iter->tag_id, media.id);
	}

	//delete all entries associated with non-existent tag id
	if (link_delete_list_by_tag_id.size() > 0) {
		for (auto iter = link_delete_list_by_tag_id.begin(); iter != link_delete_list_by_tag_id.end(); iter++) {

			Logger::Log("Deleting tag link from db with tag id " % QString::number(*iter));

			ret = tag_link_db.RemoveTagLinkByTagId(*iter);
			if (ret < 0) {
				//couldn't remove this link? what to even do if it fails?
			}
		}
	}

	//delete all entries associated with non-existent media id
	if (link_delete_list_by_media_id.size() > 0) {
		for (auto iter = link_delete_list_by_media_id.begin(); iter != link_delete_list_by_media_id.end(); iter++) {

			Logger::Log("Deleting tag link from db with media id " % QString::number(*iter));

			ret = tag_link_db.RemoveTagLinkByMediaId(*iter);
			if (ret < 0) {
				//couldn't remove this link? what to even do if it fails?
			}
		}
	}
	
	Logger::Log("Tag links resolved", LogEntry::LT_SUCCESS);

	return 1;
}

int
Daemon::ValidateMedia(QVector<MediaInfo>& db_media_vec, QVector<MediaInfo>* soft_delete_media_vec) {

	auto iter = db_media_vec.begin();
	while (iter != db_media_vec.end()) {

		QString sub_path_name = iter->GetSubpathLongName();

		if (!PathUtil::FileExistsW((abs_root_dir + sub_path_name).toStdWString())) {
			soft_delete_media_vec->push_back(std::move(*iter));

			iter = db_media_vec.erase(iter);
			continue;
		}
		
		if (ignore_list.MatchIgnore(sub_path_name)) {		

			//this media did not change location in file system but the dirs it resides in or this media specifically was ignored due to a new setting in ignorefile
			
			media_db.RemoveMedia(iter->id);		//remove from media db
												//remove one at a time as probability of many media becomes invalid because of new ignore entry is low

			tag_link_db.RemoveTagLinkByMediaId(iter->id);

												//TODO:: monitor this in the future and see if it's common or not

			Logger::Log("Media id: " % QString::number(iter->id) % " name: " % iter->long_name % " deleted from database. (IGNORED)", LogEntry::LT_WARNING);
			iter = db_media_vec.erase(iter);
			continue;
		}

		global_media_list.InsertMedia(*iter);	//TODO:: add move semantic to media list
		iter++;
	}

	Logger::Log("Media validation complete", LogEntry::LT_SUCCESS);

	if (!soft_delete_media_vec->empty()) {
		Logger::Log(QString::number(soft_delete_media_vec->size()) % " media soft deleted.");
	}

	return 1;
}

int
Daemon::LoadIgnoreList() {

	if (ignore_list.LoadIgnoreFile(working_subdir) < 0) {
		Logger::Log("Failed to load ignore file", LogEntry::LT_ERROR);
		return -1;
	}

	Logger::Log("Ignore file loaded", LogEntry::LT_SUCCESS);
	return 1;
}

int 
Daemon::LoadFilenameToTagMap() {

	if (mediamap.LoadFile() < 0) {
		Logger::Log("Failed load filenamemap file", LogEntry::LT_ERROR);
		return -1;
	}

	Logger::Log("filename map initialized", LogEntry::LT_SUCCESS);
	return 1;
}

int
Daemon::DiscoverNewMedia(QVector<MediaInfo>& new_media_list) {

	QStack<QString> dir_stack;
	QString curr_dir;
	HANDLE find_handle;
	WIN32_FIND_DATAW find_data_buff;

	curr_dir = abs_root_dir;
	curr_dir.append("\\*");
	dir_stack.push(curr_dir);

	while (!dir_stack.empty()) {
		curr_dir = dir_stack.top();
		dir_stack.pop();

		MediaInfo m_info_buff;

		//list files
		std::wstring tmp = curr_dir.toStdWString();
		find_handle = FindFirstFileW(tmp.c_str(), &find_data_buff);
		if (find_handle == INVALID_HANDLE_VALUE) {

			//something wrong
			return -1;
		}

		
		curr_dir.chop(1);	//remove * from curr dir so C:\dir\* becomes C:\dir\ 
		curr_dir.chop(1);	//remove \ from the end

		do {

			//ignore . and .. directories
			if (!lstrcmpW(find_data_buff.cFileName, L".") || !lstrcmpW(find_data_buff.cFileName, L"..")) {
				continue;
			}

			//is directory
			if (find_data_buff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {

				//add this name to file tracker
				file_tracker.AddDirAbsPath(curr_dir, QString::fromStdWString(find_data_buff.cFileName), QString::fromStdWString(find_data_buff.cAlternateFileName));

				QString tmp_dir = curr_dir;
				tmp_dir.push_back('\\');
				tmp_dir.append(QString::fromStdWString(find_data_buff.cFileName));

				//if this dir is ignored then do not traverse
				if (ignore_list.MatchIgnoreDir(tmp_dir.mid(abs_root_dir.size()))) {
					continue;
				}

				//add to traverse stack
				tmp_dir.append("\\*");
				dir_stack.push(tmp_dir);
				continue;
			}

			QString sub_path = curr_dir.mid(abs_root_dir.size());
			//if sub path is just \ then that's no subpath at all because that means this media
			//would be in root directory without a subpath
			if (sub_path.size() == 1) {
				sub_path.clear();
			}

			QString sub_path_name = sub_path;
			sub_path_name.push_back(L'\\');
			sub_path_name.append(QString::fromStdWString(find_data_buff.cFileName));

			//QString res = sub_path_name;

			//this media was found in lookup table
			if (global_media_list.MediaExistBySubpathName(sub_path_name)) {
				//res = res % "Exist";
				continue;
			}

			//skip this file if ignored
			if (ignore_list.MatchIgnore(sub_path_name)) {
				continue;
			}

			//Logger::Log(res);
		
			m_info_buff.sub_path = sub_path;
			m_info_buff.long_name = QString::fromStdWString(std::wstring(find_data_buff.cFileName));
			m_info_buff.short_name = QString::fromStdWString(std::wstring(find_data_buff.cAlternateFileName));
			new_media_list.push_back(m_info_buff);

		} while (FindNextFileW(find_handle, &find_data_buff) != 0);

		//any error other than no more files is something wrong
		if (GetLastError() != ERROR_NO_MORE_FILES) {
			return -2;
		}

		FindClose(find_handle);
	}

	QtConcurrent::blockingMap(new_media_list, [this](MediaInfo& media) -> void {

		if (FileUtil::GetFileSHA2(this->abs_root_dir, media) < 0) {
			Logger::Log("Failed to calculate hash for file: " % media.long_name, LogEntry::LT_WARNING);
			media.hash.clear();
		}
	});

	Logger::Log("New media discovery complete", LogEntry::LT_SUCCESS);
	return 1;
}

int 
Daemon::ResolveNewAndSoftDeletedMedia(QVector<MediaInfo>& soft_delete_media_vec, QVector<MediaInfo>& new_media_vec) {
	
	//TODO: find out how big is new media or deleted media assumption is they are small enough to use a naive linear search
	auto find_hash = [&new_media_vec](const QString& hash, int* idx_out) -> bool {
		for (int i = 0; i < new_media_vec.size(); i++) {
			if (new_media_vec[i].hash == hash) {

				*idx_out = i;
				return true;
			}
		}
		return false;
	};

	auto soft_delete_iter = soft_delete_media_vec.begin();
	QVector<unsigned int> id_list;									//used to remove media from db
	while (soft_delete_iter != soft_delete_media_vec.end()) {

		int idx;
		if (find_hash(soft_delete_iter->hash, &idx)) {

			
			new_media_vec[idx].id = soft_delete_iter->id;
			global_media_list.InsertMedia(new_media_vec[idx]);	//TODO: move semantic in the future
			media_db.UpdateMedia(new_media_vec[idx]);
			Logger::Log("Media id: " % QString::number(soft_delete_iter->id) % " resolved. subpath: " % soft_delete_iter->sub_path % " -> " % new_media_vec[idx].sub_path
																						  % ", name: " % soft_delete_iter->long_name % " -> " % new_media_vec[idx].long_name);

			new_media_vec.remove(idx);											//new media will no longer hold it as new_media_vec will be used to add to medialist later 
			soft_delete_iter = soft_delete_media_vec.erase(soft_delete_iter);	//remove resolved media from soft_delete_vec as the remaining media will be deleted from db later
			continue;
		}

		id_list.push_back(soft_delete_iter->id);
		soft_delete_iter++;
	}
	
	//delete unresolved from db
	if (!soft_delete_media_vec.empty()) {

		if (id_list.size() == 1) {
			if (media_db.RemoveMedia(id_list[0]) < 0) {
				goto db_err;
			}

			if (tag_link_db.RemoveTagLinkByMediaId(id_list[0]) < 0) {
				goto db_err;
			}
		}
		else if (id_list.size() > 1) {
			if (media_db.RemoveMediaList(id_list) < 0) {
				goto db_err;
			}

			if (tag_link_db.RemoveTagLinkByMediaIdList(id_list) < 0) {
				goto db_err;
			}
		}

		for (const MediaInfo& m_info : soft_delete_media_vec) {
			Logger::Log("Media id: " % QString::number(m_info.id) % " name: " % m_info.long_name % " removed", LogEntry::LT_SUCCESS);
		}
	}
	
	Logger::Log("Soft deleted media resolved", LogEntry::LT_SUCCESS);
	return 1;

db_err:
	Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
	return -Error::DAEMON_DB;
}

int 
Daemon::PopulateDirMediaId() {

	QVector<Media*> media_list;
	global_media_list.GetAllMediaPtr(&media_list);

	for (const Media* m_info : media_list) {
		file_tracker.AddMediaSubPath(m_info->sub_path, m_info->id);
	}

	Logger::Log("File tracker loaded", LogEntry::LT_SUCCESS);
	return 1;
}

int 
Daemon::ProcessNotifyEvent(const NotifyEvent& event, Notify& notifier) {


	QString file_name = QString::fromStdWString(event.file_wname);
	QString sub_path = QString::fromStdWString(event.dir_wname);
	QString sub_path_name;
	QString full_path = abs_root_dir;

	MediaInfo media_buff;
	bool dir_flag = false;

	if (!sub_path.isEmpty()) {

		//if this sub path is not being tracked, it was ignored 
		if (!file_tracker.DirExist(sub_path)) {
			return 1;
		}

		//expand path component short name if possible
		file_tracker.GetPathLongName(sub_path, &sub_path);

		sub_path_name = sub_path;
		full_path.append(sub_path_name);
	}

	full_path.append('\\');
	sub_path_name.append('\\');
	sub_path_name.append(file_name);
	full_path.append(file_name);

	//skip processing if this file is ignored
	if (ignore_list.MatchIgnore(sub_path_name)) {
		return 1;
	}

	if (event.event == NotifyEvent::REMOVE || event.event == NotifyEvent::RENAME_OLD || event.event == NotifyEvent::MODIFY) {
	
		dir_flag = file_tracker.DirExist(sub_path_name);

		if (!dir_flag) {
			//media must exist in daemon already get media id
			media_list_lock.lockForRead();

			//this media doesnt exist in daemon, this file was somehow never tracked
			if (!global_media_list.MediaExistBySubpathName(sub_path_name)) {
				return -1;
			}

			else {
				global_media_list.GetMediaInfoBySubpathName(sub_path_name, &media_buff);
			}

			media_list_lock.unlock();
		}
		else {
			file_tracker.GetDirName(sub_path_name, &media_buff.long_name, &media_buff.short_name);
		}
	}

	//need to take care of the previous soft remove first
	if (file_tracker.soft_del_flag && event.event != NotifyEvent::CREATE) {
		if (file_tracker.soft_del_dir_flag) {
			RemoveDir(file_tracker.soft_del_dir_sub_path_name);
		}
		else {
			RemoveMedia(file_tracker.soft_del_media.id);
		}
		file_tracker.soft_del_flag = false;
	}

	//call file tracker event handler
	switch (event.event) {
	case NotifyEvent::CREATE: {

		Logger::Log("Evt: CREATE: " % media_buff.long_name % "\tPath: " % sub_path, LogEntry::LT_MONITOR);

		FileTracker::IsDir(full_path, &dir_flag);
		FileTracker::GetFileLongShortName(full_path, &media_buff.long_name, &media_buff.short_name);
		media_buff.sub_path = sub_path;

		if (file_tracker.soft_del_flag && (file_tracker.soft_del_dir_flag == dir_flag)) {

			if (dir_flag) {

				int slash_idx = file_tracker.soft_del_dir_sub_path_name.lastIndexOf('\\');
				QString soft_del_dir_name = file_tracker.soft_del_dir_sub_path_name;
				
				if (slash_idx != -1) {
					soft_del_dir_name = soft_del_dir_name.mid(slash_idx + 1);
				}

				
				if (soft_del_dir_name == file_name) {
					UpdateDirSubDir(file_tracker.soft_del_dir_sub_path_name, sub_path);
					file_tracker.soft_del_flag = false;
					break;
				}
			}
			else {
				QString new_file_hash;
				FileUtil::GetFileSHA2(abs_root_dir, media_buff, &new_file_hash);
				if (file_tracker.soft_del_media.long_name == media_buff.long_name && file_tracker.soft_del_media.hash == new_file_hash) {
					//move file
					UpdateMediaSubdir(file_tracker.soft_del_media.id, sub_path);
					file_tracker.soft_del_flag = false;
					break;
				}
			}

			if (file_tracker.soft_del_dir_flag) {
				RemoveDir(file_tracker.soft_del_dir_sub_path_name);
			}
			else {
				//name does not match, hard delete
				RemoveMedia(file_tracker.soft_del_media.id);
			}

			file_tracker.soft_del_flag = false;

			//dont break here because we want to perform add operation now
		}

		if (dir_flag) {
			AddDir(sub_path, media_buff.long_name, media_buff.short_name);
		}
		else {

			media_buff.id = AddMedia(sub_path, media_buff.long_name, media_buff.short_name);

			QVector<MediaInfo> tmp = { media_buff };
			FormMediaMappedLink(tmp);
		}

		break;
	}

	case NotifyEvent::REMOVE: {
		
		Logger::Log("Evt: REMOVE: " % media_buff.long_name % "\tPath: " % sub_path, LogEntry::LT_MONITOR);

		//need to make sure next event isnt create to actually delete, soft delete for now
		file_tracker.soft_del_flag = true;
		file_tracker.soft_del_dir_flag = dir_flag;

		if (dir_flag) {
			file_tracker.soft_del_dir_sub_path_name = sub_path_name;
		}
		else {
			file_tracker.soft_del_media = media_buff;
		}
		
		break;
	}

	case NotifyEvent::MODIFY: {

		//not much to do is dir is modified not even announce the event
		if (dir_flag) {
			break;
		}

		Logger::Log("Evt: MODIFY: " % media_buff.long_name % "\tPath: " % sub_path, LogEntry::LT_MONITOR);

		QString new_hash;
		FileUtil::GetFileSHA2(abs_root_dir, media_buff, &new_hash);

		media_list_lock.lockForWrite();

		global_media_list.UpdateMediaHash(media_buff.id, new_hash);

		media_list_lock.unlock();

		break;
	}

	case NotifyEvent::RENAME_OLD: {

		Logger::Log("Evt: RENAME OLD: " % media_buff.long_name % "\tPath: " % sub_path, LogEntry::LT_MONITOR);

		if (notifier.HasEvent()) {
			NotifyEvent next_event;
			notifier.PeekNextEvent(&next_event);
			if (next_event.event == NotifyEvent::RENAME_NEW) {

				//find last back slash and replace the rest with new file name
				int idx = full_path.lastIndexOf('\\');
				full_path.chop(full_path.size() - 1 - idx);
				full_path.append(QString::fromStdWString(next_event.file_wname));

				FileTracker::GetFileLongShortName(full_path, &media_buff.long_name, &media_buff.short_name);

				if (dir_flag) {
					UpdateDirName(sub_path_name, media_buff.long_name, media_buff.short_name);
				}
				else {
					//rename media
					UpdateMediaName(media_buff.id, media_buff.long_name, media_buff.short_name);
				}

				notifier.UpdateNextEventTypeToSkip();
				break;
			}
		}

		//actualy error for not having corresponding rename_new
		break;
	}

	case NotifyEvent::RENAME_NEW:

		//should be handled in rename_old case, skip
		break;
	default:
		//should never happen
		return -1;
	}

	return 1;
}

int 
Daemon::FormLink(const unsigned int tag_id, const unsigned int media_id) {

	global_tag_list.InsertTagMedia(tag_id, media_id);
	global_media_list.InsertMediaTag(tag_id, media_id);

	//update to database
	if (tag_link_db.CreateTagLink(tag_id, media_id) < 0) {
		Logger::Log(DAEMON_DB_MSG, LogEntry::LT_ERROR);
		tag_list_lock.unlock();
		media_list_lock.unlock();
		return -Error::DAEMON_DB;
	}

	tag_list_lock.unlock();
	media_list_lock.unlock();

	Tag tag_buff;
	ModelTag m_tag_buff;

	global_tag_list.GetTagById(tag_id, &tag_buff);
	tag_buff.FormModelTag(&m_tag_buff);

	emit LinkFormed(m_tag_buff, media_id);

	Logger::Log("Linked formed: Tag: " % m_tag_buff.name % " media id: " % QString::number(media_id), LogEntry::LT_SUCCESS);
	return 1;
}