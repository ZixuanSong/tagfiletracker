#pragma once

#include <QThread>
#include <QReadWriteLock>

#include <list>

#include "notify.h"
#include "db.h"
#include "config.h"
#include "logger.h"
#include "tag_list.h"
#include "media_list.h"
#include "tag_link.h"
#include "track_ignore.h"
#include "file_tracker.h"
#include "media_map.h"



//the glue that holds subsystems together:

/*
	Design decisions:

	1. Why isn't TagLinks managed like Taglist? What about holes?

	Since rowid in sqlite is 64 bit we expect taglink table to be able to
	hold up to 900 thousand trillion rows or links. But the same argument
	could be used for tags: just wait 900 thousand trillion rows are used
	and let sqlite reuse the rowids. First, tag ids should corresponds to
	vector index so we can't wait for sqlite to reach 2^31 before rowid 
	wraps back on itself, we would have inserted trillions of dummy tags for
	those holes to have vector index in sync with those 64 bit tag id. Second,
	taglink list is temporary. When media table is loaded, taglink list will
	be resolved and tag's individual pointers will be populated with media
	structure in memory. so taglink id is of no real significance aside from
	sqlite internal mangement. 

	2. Who should perform validation checks?

	It is no doubt any user controlled input should be checked for sanity.
	Therefore any function which directly interfaces with UI thread should
	check. However checking should be inexpensive, since UI checking will be
	mostly done in GUI thread. Daemon methods will ensure the validity of its
	arguments, since user could enter sanitized but invalid input. Any 
	subsequent functions beyond daemon can assume inputs are valid and no checks
	are required. A special case is when interfacing with a third party system
	such as sqlite. Validation checks should be performed according to
	potential failures as pointed out by documentation.

	3. Should there be a mechanism to cancel readdirectorychangesW async request?

	No. Current monitor loop uses completion routine to handle async requests.
	Completion routines reads APC queue for the thread which handles completion.
	But if thread terminates, the APC queued is also freed, thus any pending 
	async completion is ignored. This is the desired behavior since monitor loop
	only cares about what happens when its coresponding thread also runs. So if
	monitor thread terminates and has outstanding APC item, they are of no 
	consequence.

	4. When should GUI be notified when new tag is added or deleted.

	For adding new tag, gui should be notified when all internal processes has
	been completed (updating in memory list, updating db). Only then can the new
	tag in GUI can be properly used. For deletion, GUI item should be removed 
	immediately regardless of internal processes. That way the item can not
	be used while internal processing is happening in the background.

	5. Who is responsible for ensuring thread safety?

	Although it might be intuitive to lock at small granularities such as tag list 
	and media list methods to reduce impediment to concurrency which arises from
	locking, we will lock at daemon methods for complete thread safety. The problem
	with locking at list levels is cause by the fact that some daemon operations
	such as forming and destroying links requires both lists to remain unchanged
	until the whole operation is complete. Locking at list methods can only guarantee
	one list's atomiticity and not the other. A solution to this problem could be
	solved by exposing the internal lock of a list to the daemon so that daemon
	could call a locking method when it needs to be locked without performing direct
	manipulation on that list. This aproach would imply daemon knows when it's best
	for a list to lock therefore voids the need to list method locking which is meant
	to free daemon the burden of locking explicitly. Since daemon methods represent 
	an interface to which all maniputations to in-memory are performed, we will
	perform daemon explicit locking here. If performance is hindered too much,
	alternative locking will be devise in the future.

	6. Should critical sections include db operations?

	According to sqlite documentation, the precompiled library is already in serial mode
	thus ensuring thread safety of all database operations using sqlite API. For our 
	thread sensitive data structures namely global media list and global tag list, they
	both have dedicates locks to ensure their validity. The crux of the problem however
	lies in the fact in some cases, TOCTOU could result between data structre operation
	and db operation. Suppose in the following scenario, critical section does not include 
	db operation. Daemon thread checks tag A exist but context switches away before the db
	operation could proceed. The newly switched thread is a GUI spawned thread to delete 
	tag A. If the deletion operation completes in entirety (including db) before switching
	back to daemon thread, then the proceeding db operation in daemon thread would've written
	something invalid, namely something related to tag A when tag A no longer exist. And so
	we see data structure and db individually are thread safe, but some operations requires 
	both objects to be modified atomically. Thus it is crucial that db operations to be included
	in critical sections in the thread where validity checking is involved.
*/

class Daemon : public QThread {

	Q_OBJECT

public:
	Daemon();
	~Daemon();
	
	void Stop();

	QString GetRootDirectory();

	//tag ops 

	int AddTag(const QString& tag_name, unsigned int* new_tag_id = nullptr);

	int UpdateTagName(const unsigned int tag_id, const QString& new_name);

	int RemoveTagById(const unsigned int tag_id);

	//link ops

	int FormLinkByTagName(const QString& tag_name, const unsigned int media_id);

	int FormLinkByIds(const unsigned int tag_id, const unsigned int media_id);

	int FormMediaMappedLink(const QVector<MediaInfo>& media_list);

	int DestroyLink(const unsigned int tag_id, const unsigned int media_id);

	//media ops

	int AddMedia(const QString& sub_path, const QString& long_name, const QString& short_name, const QString& hash = QString());

	int AddMediaList(QVector<MediaInfo>& media_list);

	int UpdateMediaName(const unsigned int media_id, const QString& long_name, const QString& short_name);

	int UpdateMediaSubdir(const unsigned int media_id, const QString& new_sub_dir);

	int UpdateMediaListSubdir(const QVector<unsigned int>& media_id_list, const QVector<QString>& new_sub_dir_list);

	int RemoveMedia(const unsigned int media_id);

	int RemoveMediaList(const QVector<unsigned int>& media_id_list);

	//filetracker ops

	int AddDir(const QString& sub_path, const QString& long_name, const QString& short_name);

	int UpdateDirName(const QString& sub_path_name, const QString& long_name, const QString& short_name);
	
	int UpdateDirSubDir(const QString& sub_path_name, const QString& new_sub_dir);
	
	int RemoveDir(const QString& sub_path_name);

	//thread callbacks

	MediaModelResult	GetAllMedia();

	MediaModelResult	GetAllTaglessMedia();

	MediaModelResult	GetTagMedias(const unsigned int tag_id);

	MediaModelResult	GetQueryMedia(const QString& query);

	QVector<ModelTag>	GetMediaTags(const unsigned int media_id);

	QVector<ModelTag>	GetAllTags();

signals:

	void Initialized();

	void TagInserted(const ModelTag& new_model_tag);

	void TagNameUpdated(const unsigned int tag_id, const QString& new_name);

	void TagMediaCountUpdated(const unsigned int tag_id, const unsigned int new_count);

	void TagMediaCountInc(const unsigned int tag_id);

	void TagMediaCountDec(const unsigned int tag_id);

	void TagRemoved(const unsigned int tag_id);

	void TaglessMediaInserted(const ModelMedia& new_model_media);

	void MediaInserted(const ModelMedia& new_model_media);

	void MediaRemoved(const unsigned int media_id);

	void MediaNameUpdated(const unsigned int media_id, const QString& new_name);

	void MediaSubdirUpdated(const unsigned int media_id, const QString& new_subdir);

	void LinkFormed(const ModelTag& model_tag, const unsigned int media_id);

	void LinkDestroyed(const unsigned int tag_id, const unsigned int media_id);

protected:

	void run() override;

private:

	QString									abs_root_dir;		//everything in this absolute path directory will be tracked
																//ends without slash ex. C:\Dir1\Dir2

	QString									working_subdir;		//this sub dir in conjunction with root dir specifies the directory that houses every file this program needs 
																//to operate, it is the same dir where the executable resides
																//contains no beginning nor end slash ex. if the working directory's full path is C:\Dir1\Dir2\tracker 
																//and root dir is C:\Dir1\Dir2, then this subdir stores: tracker

	IgnoreList								ignore_list;
	FileTracker								file_tracker;
	MediaMap								mediamap;

	HANDLE									monitor_terminate_event;

	TagDatabase								tag_db;
	TagLinkDatabase							tag_link_db;
	MediaDatabase							media_db;

	QReadWriteLock							tag_list_lock;			//lock first - lock order to prevent deadlock
	QReadWriteLock							media_list_lock;		//lock second
	QReadWriteLock							file_tracker_lock;		//probabaly wont need it as file tracking will all run on the same thread

	TagList									global_tag_list;
	MediaList								global_media_list;

	//initialize daemon
	//kick start all daemon routines
	void Init();

	//initialize database structures 
	//called at the beginning of daemon init
	int InitDB();

	//resolve links by populating media and tag references
	int ResolveTagLinks(const QVector<TagLink>& tag_link_vec);

	//make sure all media loaded from media database are still valid
	int ValidateMedia(QVector<MediaInfo>& media_vec, QVector<MediaInfo>* soft_delete_media_vec);

	//parse ignore file build up ignore list
	int LoadIgnoreList();

	//initialize filename to tag mapping
	int LoadFilenameToTagMap();

	//recurssively search every directory monitored by daemon and adds new files to the database
	int DiscoverNewMedia(QVector<MediaInfo>& new_media);

	//try to match and see if media from database that doesnt exist anymore (soft deleted)'s hash matches with new media
	int ResolveNewAndSoftDeletedMedia(QVector<MediaInfo>& soft_delete_media_vec, QVector<MediaInfo>& new_media_vec);

	//inserts all media id into appropreate dir struct
	int PopulateDirMediaId();

	//handles an event from notifier
	int ProcessNotifyEvent(const NotifyEvent&, Notify&);

	//internal use of forming links
	int FormLink(const unsigned int, const unsigned int);
};