#pragma once

#include <QString>
#include <QHash>
#include <QSet>
#include <QFile>
#include <QLinkedList>
#include <memory>
#include <list>

#include "media_structs.h"
#include "logger.h"

struct DirTreeNode {
	QString long_name;
	QString short_name;

	std::list<std::unique_ptr<DirTreeNode>>		child_dir_list;	//std list since qlist cannot hold smart ptr (look into Qt smart pointers)
	QSet<unsigned int>							media_id_set;

	QHash<QString, DirTreeNode*>				child_dir_name_to_node_table;
	QHash<QString, DirTreeNode*>				child_dir_short_name_to_node_table;
};

struct DirTree {
	QString root_dir_abs_path;
	DirTreeNode root_dir_node;
};

class FileTracker
{
public:

	//annoying remove delay shit

	bool		soft_del_flag = false;
	bool		soft_del_dir_flag = false;
	QString		soft_del_dir_sub_path_name;
	MediaInfo	soft_del_media;


	enum FileAction {
		NONE,
		MOVE,
		RENAME,
		ADD,
		REMOVE
	};

	FileTracker();
	~FileTracker();

	static int GetFileLongShortName(const QString&, QString*, QString*);
	static int IsDir(const QString&, bool*);


	void SetRootDir(const QString&);

	int		AddDirAbsPath(const QString& abs_path, const QString& long_name, const QString& short_name);
	int		AddDirSubPath(const QString& sub_path, const QString& long_name, const QString& short_name);
	int		AddMediaAbsPath(const QString& abs_path, const unsigned int media_id);
	int		AddMediaSubPath(const QString& sub_path, const unsigned int media_id);
	int		RemoveMedia(const QString& sub_path, const unsigned int media_id);
	int		UpdateDirName(const QString& sub_path_name, const QString& long_name, const QString& short_name);
	int		UpdateDirSubdir(const QString& sub_path_name, const QString& new_sub_dir);
	int		RemoveDir(const QString& sub_path_name);
	bool	DirExist(const QString& sub_path_name);
	bool	DirExistAbs(const QString& abs_path);
	void	GetPathLongName(const QString& sub_path, QString* out);
	int		GetDirMediaIdRecurByPath(const QString& path, QVector<unsigned int>* out);
	int		GetDirName(const QString& sub_path_name, QString* long_name, QString* short_name);
	void	Clear();

private:

	DirTree						dir_tree;

	int	GetDirNodePtr(const QString& path, DirTreeNode** out);
	int GetDirMediaIdRecurByNode(DirTreeNode* root_node, QVector<unsigned int>* out);
};

