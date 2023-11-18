#pragma once

#define CONFIG_FILE_NAME "config"

#include <QMap>
#include <QMutex>
#include <QJsonObject>


struct LoggerConfig {
	int cache_size = 300;
	int anti_spam_timeout_msec = 1000;
};

struct ThumbnailGenConfig {
	int cache_size = 100;
	struct {
		int w = 140;
		int h = 140;
	} icon_size;
};

struct MediaListViewConfig {
	struct {
		int w = 160;
		int h = 160;
	} grid_size;

	int spacing = 1;
};

union ConfigUnion {
	LoggerConfig logger_config;
	ThumbnailGenConfig thumbnail_gen_config;
	MediaListViewConfig media_list_view_config;

	explicit ConfigUnion(LoggerConfig config):
		logger_config(std::move(config))
	{
	}

	explicit ConfigUnion(ThumbnailGenConfig config):
		thumbnail_gen_config(std::move(config))
	{
	}

	explicit ConfigUnion(MediaListViewConfig config):
		media_list_view_config(std::move(config))
	{
	}
};

class Config {
public:

	~Config();

	enum SubconfigType {
		LOGGER,
		THUMBGEN,
		MEDIAVIEW
	};
	
	int									Load();
	int									Save();
	bool								SubconfigExist(SubconfigType type);

	ConfigUnion							GetSavedSubconfig(SubconfigType type);
	QMap<SubconfigType, ConfigUnion>	GetAllSavedConfig();
	void								SetSavedConfig(SubconfigType sys_type, const ConfigUnion& config);

	ConfigUnion							GetEffectiveSubconfig(SubconfigType type);
	QMap<SubconfigType, ConfigUnion>	GetAllEffectiveConfig();
	void								SetEffectiveConfig(SubconfigType sys_type, const ConfigUnion& config);

private:

	//QMutex									config_map_lock;
	QMap<SubconfigType, ConfigUnion>		saved_config_map;
	QMap<SubconfigType, ConfigUnion>		effective_config_map;
	bool dirty = false;

	LoggerConfig		ParseLoggerConfig(const QJsonObject& logger_config_json);
	ThumbnailGenConfig	ParseThumbGenConfig(const QJsonObject& thumbgen_config_json);
	MediaListViewConfig	ParseMediaListViewConfig(const QJsonObject& media_view_config_json);

	bool StringCheck(const QJsonValue& json_val);
	bool IntCheck(const QString& str, int* out);
	bool LowerBoundCheck(int val, int bound);
};
