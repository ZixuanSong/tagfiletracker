#pragma once

#include <QVector>
#include <QString>
#include <QFile>
#include <QSet>

#include "logger.h"

#define FILENAME_MAP_FILE_NAME	"mediamap"

struct MapEntry {
	QString				pattern;
	QVector<QString>	tag_name_list;
};

struct MapBlockStatus {
	QSet<QString>	pattern_buff;
	QSet<QString>	tag_buff;
	unsigned int	line_num = 1;
	bool			pattern_flag = false;				//terrible. put it in a bitmap in the future
	bool			tag_flag = false;
	bool			seek_next_begin_flag = false;
};

class MediaMap
{
public:
	MediaMap();
	~MediaMap();

	int				LoadFile();
	void			GetMappedTags(const QString& media_subpathname, QVector<QString>* out);
	inline	int		GetSize() { return map_entry_vec.size(); };
	void			Reset();

private:

	QVector<MapEntry>	map_entry_vec;

	int		ParseLine(const QString& line, MapBlockStatus* status);
	void	ProcessMapBlock(const QSet<QString>& pattern_list, const QSet<QString>& tag_list );
};

