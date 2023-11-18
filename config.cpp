#include "pch.h"
#include "config.h"

#include <QFile>
#include <QStringBuilder>
#include <QJsonDocument>
#include <QJsonValue>
#include <QPair>
#include <QDebug>

#include "error.h"
#include "logger.h"

Config::~Config() {
	if (dirty) {
		Save();
	}
}

int
Config::Load() {

	Logger::Log("Loading config file", LogEntry::LT_ATTN);

	//config_map_lock.lock();

	QFile file(CONFIG_FILE_NAME);
	if (!file.open(QIODevice::ReadOnly)) {
		Logger::Log(QFILE_OPEN_MSG, LogEntry::LT_ERROR);
		//config_map_lock.unlock();
		return -Error::QFILE_OPEN;
	}

	QJsonParseError json_err;
	QJsonDocument json = QJsonDocument::fromJson(file.readAll(), &json_err);
	file.close();
	//config_map_lock.unlock();
	
	if (json.isNull()) {
		Logger::Log(json_err.errorString(), LogEntry::LT_ERROR);
		return -Error::QJSON_PARSE;
	}

	if (!json.isObject()) {
		Logger::Log("Document not a json object", LogEntry::LT_ERROR);
		return -Error::QJSON_PARSE;
	}

	QJsonObject all_config_obj = json.object();

	//config_map_lock.lock();
	for (const QString& sub_sys_key : all_config_obj.keys()) {
		if (!all_config_obj.value(sub_sys_key).isObject()) {
			Logger::Log("Subconfig: " % sub_sys_key % " is not a json object", LogEntry::LT_WARNING);
			continue;
		}

		Logger::Log("Parsing subconfig " % sub_sys_key % "...");

		if (sub_sys_key == "logger") {
			saved_config_map.insert(LOGGER, ConfigUnion( ParseLoggerConfig(all_config_obj.value(sub_sys_key).toObject()) ));
			effective_config_map.insert(LOGGER, *saved_config_map.constFind(LOGGER));
		} 
		else if (sub_sys_key == "thumbgen") {
			saved_config_map.insert(THUMBGEN, ConfigUnion( ParseThumbGenConfig(all_config_obj.value(sub_sys_key).toObject()) ));
			effective_config_map.insert(THUMBGEN, *saved_config_map.constFind(THUMBGEN));
		}
		else if (sub_sys_key == "mediaview") {
			saved_config_map.insert(MEDIAVIEW, ConfigUnion(ParseMediaListViewConfig(all_config_obj.value(sub_sys_key).toObject())));
			effective_config_map.insert(MEDIAVIEW, *saved_config_map.constFind(MEDIAVIEW));
		}
		else {
			Logger::Log("Unknown subconfig, skipping...", LogEntry::LT_WARNING);
			continue;
		}
	}

	if (!saved_config_map.contains(LOGGER)) {
		saved_config_map.insert(LOGGER, ConfigUnion(LoggerConfig()));
		effective_config_map.insert(LOGGER, ConfigUnion(LoggerConfig()));
	}

	if (!saved_config_map.contains(THUMBGEN)) {
		saved_config_map.insert(THUMBGEN, ConfigUnion(ThumbnailGenConfig()));
		effective_config_map.insert(THUMBGEN, ConfigUnion(ThumbnailGenConfig()));
	}

	if (!saved_config_map.contains(MEDIAVIEW)) {
		saved_config_map.insert(MEDIAVIEW, ConfigUnion(MediaListViewConfig()));
		effective_config_map.insert(MEDIAVIEW, ConfigUnion(MediaListViewConfig()));
	}

	//config_map_lock.unlock();
	Logger::Log("Config file loaded", LogEntry::LT_SUCCESS);
	return 1;
}

int
Config::Save() {
	Logger::Log("Saving to config file", LogEntry::LT_ATTN);

	QJsonObject main_obj;
	//config_map_lock.lock();
	{
		QJsonObject logger_config_obj;
		logger_config_obj.insert("cache_size", QString::number(saved_config_map.constFind(LOGGER)->logger_config.cache_size));
		logger_config_obj.insert("spam_timeout_msec", QString::number(saved_config_map.constFind(LOGGER)->logger_config.anti_spam_timeout_msec));
		main_obj.insert("logger", QJsonValue(std::move(logger_config_obj)));
	}
	{
		QJsonObject thumbgen_config_obj;
		thumbgen_config_obj.insert("cache_size", QString::number(saved_config_map.constFind(THUMBGEN)->thumbnail_gen_config.cache_size));
		thumbgen_config_obj.insert("icon_width", QString::number(saved_config_map.constFind(THUMBGEN)->thumbnail_gen_config.icon_size.w));
		thumbgen_config_obj.insert("icon_height", QString::number(saved_config_map.constFind(THUMBGEN)->thumbnail_gen_config.icon_size.h));
		main_obj.insert("thumbgen", QJsonValue(std::move(thumbgen_config_obj)));
	}
	{
		QJsonObject media_list_view_config_obj;
		media_list_view_config_obj.insert("grid_width", QString::number(saved_config_map.constFind(MEDIAVIEW)->media_list_view_config.grid_size.w));
		media_list_view_config_obj.insert("grid_height", QString::number(saved_config_map.constFind(MEDIAVIEW)->media_list_view_config.grid_size.h));
		media_list_view_config_obj.insert("spacing", QString::number(saved_config_map.constFind(MEDIAVIEW)->media_list_view_config.spacing));
		main_obj.insert("mediaview", QJsonValue(std::move(media_list_view_config_obj)));
	}

	//config_map_lock.unlock();

	QByteArray data = QJsonDocument(std::move(main_obj)).toJson(QJsonDocument::Indented);

	//config_map_lock.lock();

	QFile file(CONFIG_FILE_NAME);
	if (!file.open(QIODevice::WriteOnly)) {
		Logger::Log(QFILE_OPEN_MSG, LogEntry::LT_ERROR);
		//config_map_lock.unlock();
		return -Error::QFILE_OPEN;
	}

	file.write(data);

	//config_map_lock.unlock();

	Logger::Log("Config file saved", LogEntry::LT_SUCCESS);
	return 1;
}

bool
Config::SubconfigExist(SubconfigType type) {
	//config_map_lock.lock();
	bool exist = saved_config_map.contains(type);
	//config_map_lock.unlock();
	return exist;
}

ConfigUnion
Config::GetSavedSubconfig(Config::SubconfigType type) {
	//config_map_lock.lock();

	ConfigUnion config = *(saved_config_map.constFind(type));

	//config_map_lock.unlock();
	return config;
}

QMap<Config::SubconfigType, ConfigUnion>	
Config::GetAllSavedConfig() {

	//config_map_lock.lock();

	QMap<Config::SubconfigType, ConfigUnion> config = saved_config_map;	//copy

	//config_map_lock.unlock();

	return config;
}

void
Config::SetSavedConfig(SubconfigType type, const ConfigUnion& config) {
	//config_map_lock.lock();

	saved_config_map.insert(type, config);

	//config_map_lock.unlock();

	dirty = true;
}

ConfigUnion							
Config::GetEffectiveSubconfig(SubconfigType type) {
	return *(effective_config_map.constFind(type));
}

QMap<Config::SubconfigType, ConfigUnion>
Config::GetAllEffectiveConfig() {
	return effective_config_map;
}

void							
Config::SetEffectiveConfig(SubconfigType sys_type, const ConfigUnion& config) {
	effective_config_map.insert(sys_type, config);
}

//private

LoggerConfig
Config::ParseLoggerConfig(const QJsonObject& logger_config_json) {
	LoggerConfig config;
	QString str_buff;
	for (const QString& key : logger_config_json.keys()) {

		if (!logger_config_json.value(key).isString()) {
			Logger::Log("Key: " % key % " value is not a string. Using default value...", LogEntry::LT_WARNING);
			continue;
		}

		str_buff = logger_config_json.value(key).toString();

		if (key == "cache_size") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.cache_size = input;
			}
		}
		else if (key == "spam_timeout_msec") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.anti_spam_timeout_msec = input;
			}
		}
		else {
			Logger::Log("Unknown key: " % key % ". Skipping...", LogEntry::LT_WARNING);
		}
	}
	
	return config;
}

ThumbnailGenConfig
Config::ParseThumbGenConfig(const QJsonObject& thumbgen_config_json) {
	ThumbnailGenConfig config;
	QString str_buff;
	for (const QString& key : thumbgen_config_json.keys()) {

		if (!thumbgen_config_json.value(key).isString()) {
			Logger::Log("Key: " % key % " value is not a string. Using default value...", LogEntry::LT_WARNING);
			continue;
		}

		str_buff = thumbgen_config_json.value(key).toString();

		if (key == "cache_size") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.cache_size = input;
			}
		}
		else if (key == "icon_width") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.icon_size.w = input;
			}
		}
		else if (key == "icon_height") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.icon_size.h = input;
			}
		}
		else {
			Logger::Log("Unknown key: " % key % ". Skipping...", LogEntry::LT_WARNING);
		}
	}
	
	return config;
}

MediaListViewConfig	
Config::ParseMediaListViewConfig(const QJsonObject& media_view_config_json) {
	MediaListViewConfig config;
	QString str_buff;
	for (const QString& key : media_view_config_json.keys()) {

		if (!media_view_config_json.value(key).isString()) {
			Logger::Log("Key: " % key % " value is not a string. Using default value...", LogEntry::LT_WARNING);
			continue;
		}

		str_buff = media_view_config_json.value(key).toString();

		if (key == "grid_width") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.grid_size.w = input;
			}
		}
		else if (key == "grid_height") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.grid_size.h = input;
			}
		} 
		else if (key == "spacing") {
			int input;
			if (IntCheck(str_buff, &input) && LowerBoundCheck(input, 0)) {
				config.spacing = input;
			}
		}
		else {
			Logger::Log("Unknown key: " % key % ". Skipping...", LogEntry::LT_WARNING);
		}
	}

	return config;
}

bool 
Config::IntCheck(const QString& str, int* out) {
	bool ok;
	*out = str.toInt(&ok, 10);	//always base 10 conversion

	if (!ok) {
		Logger::Log("Value: " % str % " cannot be converted to int. Using default value...", LogEntry::LT_WARNING);
	}

	return ok;
}

bool 
Config::LowerBoundCheck(int val, int bound) {
	if (val < bound) {
		Logger::Log("Value: " % QString::number(val) % " is less than lower bound " % QString::number(bound) % ". Using default value...", LogEntry::LT_WARNING);
		return false;
	}
	return true;
}