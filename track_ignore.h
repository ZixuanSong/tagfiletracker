#pragma once

#include <QString>
#include <QSet>
#include <QVector>
#include <QHash>
#include <QFile>

#include "logger.h"

#define DEFAULT_IGNORE_FILE_CONTENT "#This is an auto generated ignore file\n"\
									"#It has been populated with program's directory to exclude tracking program files\n"\
									"#Use pound sign (#) at the beginning of a line to escape it as comment\n"\
									"#To ignore an entire directory, make sure to end the path with a slash\n"\
									"#ex. dir1/dir2/ ignores directory dir2\n"\
									"#ex. dir1/dir2 ignores file dir2 in directory dir1\n"\
									"\n"

#define IGNORE_FILE_NAME	"ignorefile"

struct IgnoreEntry {
	QVector<QString>	sub_path_vec;
	bool				ignore_dir;
};


class IgnoreList
{
public:
	static bool		MatchWildcard(const QString&, const QString&);
	static bool		IsConcrete(const QString&);

	IgnoreList();
	~IgnoreList();


	
	int		LoadIgnoreFile(const QString& working_dir);
	int		GenerateDefaultIgnoreFile(QFile& ignore_file, const QString& working_dir);
	int		ParseIgnoreFileLine(const QString&);
	bool	MatchIgnore(const QString& path);
	bool	MatchIgnoreDir(const QString& dir_path);
	
	void	Reset();

private:

	QVector<IgnoreEntry>		ignore_entry_vec;

	QHash<QString, QList<int>>	concrete_to_index_list_table;
	QHash<QString, QList<int>>	wildcard_to_index_list_table;

};

