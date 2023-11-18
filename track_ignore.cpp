#include "track_ignore.h"
#include "error.h"

//static

bool
IgnoreList::IsConcrete(const QString& str) {
	for (QChar c : str) {
		if (c == '*') {
			return false;
		}
	}

	return true;
}

bool
IgnoreList::MatchWildcard(const QString& pattern, const QString& matchee) {
	//a single * matches all non empty string
	if (pattern.size() == 1 && pattern[0] == '*') {
		return true;
	}

	int pattern_idx = 0;
	int matchee_idx = 0;

	while (pattern_idx != pattern.size()) {

		//matchee finished before pattern means mismatch
		if (matchee_idx == matchee.size()) {
			return false;
		}

		if (pattern[pattern_idx] != '*') {
			if (matchee[matchee_idx] != pattern[pattern_idx]) {
				return false;
			}

			pattern_idx++;
			matchee_idx++;
			continue;
		}

		//pattern idx currently lands on wildcard

		//QChar lookahead_c = pattern[pattern_idx];
		//int lookahead_idx = pattern_idx;
		//look ahead to next non wildcard char
		while (pattern_idx < pattern.size() && pattern[pattern_idx] == '*') {
			pattern_idx++;
		}

		//reaching end of pattern means wildcard at the end matches everything
		if (pattern_idx == pattern.size()) {
			return true;
		}

		//let matchee catch up to lookahead char before matching again
		while (matchee_idx < matchee.size() && matchee[matchee_idx] != pattern[pattern_idx]) {
			matchee_idx++;
		}
	}

	//still characters left in matchee means pattern did not match
	if (matchee_idx != matchee.size()) {
		return false;
	}

	return true;
}

IgnoreList::IgnoreList()
{
}


IgnoreList::~IgnoreList()
{
}

int
IgnoreList::LoadIgnoreFile(const QString& working_dir) {
	QFile ignore_file(IGNORE_FILE_NAME);

	//create the default ignore file and finish
	if (!ignore_file.exists()) {
		if (!ignore_file.open(QIODevice::ReadWrite)) {
			Logger::Log(QFILE_OPEN_MSG, LogEntry::LT_ERROR);
			return -Error::QFILE_OPEN;
		}

		GenerateDefaultIgnoreFile(ignore_file, working_dir);
	}
	else {
		if (!ignore_file.open(QIODevice::ReadOnly)) {
			Logger::Log(QFILE_OPEN_MSG, LogEntry::LT_ERROR);
			return -Error::QFILE_OPEN;
		}
	}

	QTextStream t_stream(&ignore_file);
	QString line_buf;
	while (!t_stream.atEnd()) {
		line_buf = t_stream.readLine();

		ParseIgnoreFileLine(line_buf);
	}

	ignore_file.close();

	return 1;
}

int
IgnoreList::GenerateDefaultIgnoreFile(QFile& ignore_file, const QString& working_dir) {

	QString default_ignore_content = DEFAULT_IGNORE_FILE_CONTENT;

	//ignore working directory by default
	default_ignore_content.append(working_dir);
	default_ignore_content.push_back("/\n");

	ignore_file.write(default_ignore_content.toUtf8());
	ignore_file.seek(0);	//file will be loaded afterwards so reset curr byte pointer

	return 1;
}

int
IgnoreList::ParseIgnoreFileLine(const QString& line) {
	QString new_line = line.trimmed();

	//skip empty line
	if (new_line.isEmpty()) {
		return 1;
	}

	//skip comment line
	if (new_line[0] == '#') {
		return 1;
	}

	QVector<QString> sub_path_vec_buf;
	IgnoreEntry ignore_ent_buf;
	QString str_buf;
	QChar c_buf;


	ignore_ent_buf.ignore_dir = false;


	for (int i = 0; i < new_line.size(); i++) {
		c_buf = new_line[i];

		//encountered dir separator
		if (c_buf == '/') {

			str_buf = str_buf.trimmed();

			//str_buf empty means no character between separators, malformed entry
			if (str_buf.size() == 0) {
				return -1;
			}

			//at the end of line means this entry intends to ignore whole dir
			if (i == (new_line.size() - 1)) {
				ignore_ent_buf.ignore_dir = true;
			}

			sub_path_vec_buf.push_back(str_buf);
			str_buf.clear();
			continue;
		}
		
		str_buf.push_back(c_buf);
	}

	//line finished reading but still have content in str_buf
	//happens when line did not end with dir separator /
	if (str_buf.size() != 0) {
		sub_path_vec_buf.push_back(str_buf);
	}

	//empty ignore_entry_buf means malformed line 
	if (sub_path_vec_buf.size() == 0) {
		return -2;
	}

	//populate first index look up tables
	QHash<QString, QList<int>> *table_ptr;
	if (IsConcrete(sub_path_vec_buf[0])) {
		table_ptr = &concrete_to_index_list_table;
	}
	else {
		table_ptr = &wildcard_to_index_list_table;
	}

	auto table_iter = table_ptr->find(sub_path_vec_buf[0]);
	if (table_iter == table_ptr->end()) {
		QList<int> new_list;
		new_list.push_back(ignore_entry_vec.size());
		table_ptr->insert(sub_path_vec_buf[0], new_list);
	}
	else {
		table_iter->push_back(ignore_entry_vec.size());
	}

	ignore_entry_vec.push_back(ignore_ent_buf);
	ignore_entry_vec.back().sub_path_vec = std::move(sub_path_vec_buf);
	
	return 1;
}

bool
IgnoreList::MatchIgnore(const QString& matchee_path) {

	//NOTE: machee_path dir separator is backslah (\) due to windows convention
	QStringList sub_path_list = matchee_path.split("\\", QString::SkipEmptyParts);

	//get the matching entries' indexes of first sub path
	QList<int> entry_index_list;
	if (concrete_to_index_list_table.contains(sub_path_list[0])) {
		entry_index_list = concrete_to_index_list_table.value(sub_path_list[0]);
	}

	//try match with wildcard indexs 
	for (auto iter = wildcard_to_index_list_table.begin(); iter != wildcard_to_index_list_table.end(); iter++) {
		if (MatchWildcard(iter.key(), sub_path_list[0])) {
			entry_index_list.append(iter.value());
		}
	}

	//first subpath did not match anything means matchee is not in ignore list
	if (entry_index_list.size() == 0) {
		return false;
	}


	QList<int> new_entry_index_list;
	for (int i = 0; i < sub_path_list.size(); i++) {
		for (int entry_index : entry_index_list) {

			//we are at the end of this ignore entry
			if (ignore_entry_vec[entry_index].sub_path_vec.size() - 1 == i) {

				//this ignore entry ignores a dir so we check if matchee is also within this dir
				if (ignore_entry_vec[entry_index].ignore_dir) {

					//last sub path of matchee means we are looking at a file not a dir
					if (i == sub_path_list.size() - 1) {
						continue;
					}

					if (MatchWildcard(ignore_entry_vec[entry_index].sub_path_vec[i], sub_path_list[i])) {
						return true;
					}

					//it doesnt, move onto next entry
					continue;
				}

				//this ignore entry ignores a file
				
				if (i != sub_path_list.size() - 1) {
					//matchee is deeper than current path depth
					continue;
				}

				if (MatchWildcard(ignore_entry_vec[entry_index].sub_path_vec[i], sub_path_list[i])) {
					return true;
				}
				continue;
			}

			if (MatchWildcard(ignore_entry_vec[entry_index].sub_path_vec[i], sub_path_list[i])) {
				new_entry_index_list.append(entry_index);
			}
		}

		//no more entries to continue matching means matchee should not be ignored
		if (new_entry_index_list.size() == 0) {
			return false;
		}

		entry_index_list = std::move(new_entry_index_list);
		new_entry_index_list.clear();
	}

	return false;
}

bool	
IgnoreList::MatchIgnoreDir(const QString& matchee_dir_path) {
	//NOTE: machee_path dir separator is backslah (\) due to windows convention
	QStringList sub_path_list = matchee_dir_path.split("\\", QString::SkipEmptyParts);

	//get the matching entries' indexes of first sub path
	QList<int> entry_index_list;
	if (concrete_to_index_list_table.contains(sub_path_list[0])) {

		for (int idx : concrete_to_index_list_table.value(sub_path_list[0])) {
			if (ignore_entry_vec[idx].ignore_dir) {
				entry_index_list.append(idx);
			}
		 }
	}

	//try match with wildcard indexs 
	for (auto iter = wildcard_to_index_list_table.begin(); iter != wildcard_to_index_list_table.end(); iter++) {
		if (MatchWildcard(iter.key(), sub_path_list[0])) {

			for (int idx : iter.value()) {
				if (ignore_entry_vec[idx].ignore_dir) {
					entry_index_list.append(idx);
				}
			}
		}
	}

	//first subpath did not match anything means matchee is not in ignore list
	if (entry_index_list.size() == 0) {
		return false;
	}

	QList<int> new_entry_index_list;
	for (int i = 0; i < sub_path_list.size(); i++) {
		for (int entry_index : entry_index_list) {

			//we are at the end of this ignore entry
			if (ignore_entry_vec[entry_index].sub_path_vec.size() - 1 == i) {

				//if path component matches then the matchee is to be ignored
				if (MatchWildcard(ignore_entry_vec[entry_index].sub_path_vec[i], sub_path_list[i])) {
					return true;
				}

				continue;

			}

			if (MatchWildcard(ignore_entry_vec[entry_index].sub_path_vec[i], sub_path_list[i])) {
				new_entry_index_list.append(entry_index);
			}
		}

		//no more entries to continue matching means matchee should not be ignored
		if (new_entry_index_list.size() == 0) {
			return false;
		}

		entry_index_list = std::move(new_entry_index_list);
		
		//probably redundant
		new_entry_index_list.clear();
	}

	return false;
}

void
IgnoreList::Reset() {

	ignore_entry_vec.clear();
	concrete_to_index_list_table.clear();
	wildcard_to_index_list_table.clear();
}


//private
