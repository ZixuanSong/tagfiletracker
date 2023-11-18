#include "media_map.h"
#include "error.h"

#include <QRegularExpression>
#include <QStringBuilder>


MediaMap::MediaMap() 
{
}


MediaMap::~MediaMap()
{
}

int		
MediaMap::LoadFile() {
	QFile file(FILENAME_MAP_FILE_NAME);
	if (!file.open(QIODevice::ReadOnly)) {
		Logger::Log(QFILE_OPEN_MSG, LogEntry::LT_ERROR);
		return -Error::QFILE_OPEN;
	}

	MapBlockStatus status;
	//int ret;

	QTextStream t_stream(&file);
	QString line_buff;

	while (!t_stream.atEnd()) {
		
		line_buff.clear();

		t_stream.readLineInto(&line_buff);

		ParseLine(line_buff, &status);

		status.line_num++;
	}

	file.close();
	return 1;
}

void	
MediaMap::GetMappedTags(const QString& media_subpathname, QVector<QString>* out) {
	QRegularExpression regex;
	
	for (auto iter = map_entry_vec.cbegin(); iter != map_entry_vec.cend(); iter++) {
		regex.setPattern(iter->pattern);

		if (!regex.match(media_subpathname).hasMatch()) {
			continue;
		}

		for (auto tag_iter = iter->tag_name_list.cbegin(); tag_iter != iter->tag_name_list.cend(); tag_iter++) {
			if (!out->contains(*tag_iter)) {
				out->push_back(*tag_iter);
			}
		}
	}
}


void	
MediaMap::Reset() {
	map_entry_vec.clear();
}

//private

int		
MediaMap::ParseLine(const QString& line, MapBlockStatus* status) {

	if (line.isEmpty() || line[0] == '#') {
		//skip comment (#) lines
		return 1;
	}

	if (line == "BEGIN") {

		if (status->seek_next_begin_flag) {
			//reset everything processed in status so far and start over

			status->seek_next_begin_flag = false;
			status->pattern_flag = true;
			status->tag_flag = false;

			status->pattern_buff.clear();
			status->tag_buff.clear();

			return 1;
		}


		if (status->pattern_flag || status->tag_flag) {
			Logger::Log(QString::number(status->line_num) % ": " % QString(FILENAME_MAP_INVALID_BLOCK_MSG), LogEntry::LT_WARNING);
			return -Error::FILENAME_MAP_INVALID_BLOCK;
		}

		status->pattern_flag = true;
		return 1;
	}

	if (status->seek_next_begin_flag) {
		//current line is not begin but we are interested in next begin keyword
		return 1;
	}

	if (line == "TAG") {
		if (!status->pattern_flag || status->tag_flag || status->pattern_buff.empty()) {
			Logger::Log(QString::number(status->line_num) % ": " % QString(FILENAME_MAP_INVALID_BLOCK_MSG), LogEntry::LT_WARNING);
			return -Error::FILENAME_MAP_INVALID_BLOCK;
		}

		status->pattern_flag = false;
		status->tag_flag = true;
		return 1;
	}

	if (line == "END") {
		if (!status->tag_flag || status->tag_buff.empty()) {
			Logger::Log(QString::number(status->line_num) % ": " % QString(FILENAME_MAP_INVALID_BLOCK_MSG), LogEntry::LT_WARNING);
			return -Error::FILENAME_MAP_INVALID_BLOCK;
		}

		status->pattern_flag = false;
		status->tag_flag = false;

		ProcessMapBlock(status->pattern_buff, status->tag_buff);

		status->pattern_buff.clear();
		status->tag_buff.clear();
		return 1;
	}

	if (status->pattern_flag) {

		QRegularExpression regex(line);
		if (!regex.isValid()) {

			Logger::Log(QString::number(status->line_num) % ": " % QString(QREGEX_INVALID_MSG), LogEntry::LT_WARNING);
			return -Error::QREGEX_INVALID;
		}
		status->pattern_buff.insert(line);
	}
	else if (status->tag_flag) {

		QString tmp = line.trimmed();
		if (!tmp.isEmpty()) {
			status->tag_buff.insert(std::move(tmp));
		}
	}

	return 1;
}

void	
MediaMap::ProcessMapBlock(const QSet<QString>& pattern_list, const QSet<QString>& tag_list) {

	MapEntry entry_buff;
	entry_buff.tag_name_list.reserve(tag_list.size());

	for (auto iter = tag_list.begin(); iter != tag_list.end(); iter++) {
		entry_buff.tag_name_list.push_back(std::move(*iter));
	}

	for (auto iter = pattern_list.begin(); iter != pattern_list.end(); iter++) {

		entry_buff.pattern = std::move(*iter);
		map_entry_vec.push_back(entry_buff);
	}
}