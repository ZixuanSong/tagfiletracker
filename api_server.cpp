#include "api_server.h"

#include <QStringBuilder>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include <QDebug>
#include <QList>
#include <QVariant>

//static
QString
APIServerWorker::GetCommandString(const APICommand cmd) {
	static QVector<QString> str_list = {
		"ERROR",
		"OK",
		"READY",
		"GET ALL TAG",
		"GET TAG",
		"ADD TAG",
		"UPDATE TAG",
		"DELETE TAG",
		"GET MEDIA",
		"GET ROOT DIR"
	};

	return str_list[ static_cast<int>(cmd)];
}

APIServerWorker::APIServerWorker(Daemon* daemon) :
	daemon(daemon),
	pipe_server(this)
{
}

void
APIServerWorker::WorkerMain() {

	
	connect(&pipe_server, &QLocalServer::newConnection, this, &APIServerWorker::OnNewConnection);

	if (!pipe_server.listen("\\\\.\\pipe\\" % QString(PIPE_NAME))) {
		Logger::Log(pipe_server.errorString(), LogEntry::LT_ERROR);
		return;
	}

	Logger::Log("API server started", LogEntry::LT_SUCCESS);
}

void 
APIServerWorker::OnCleanup() {
	
	pipe_server.close();
	if (curr_pipe_client != nullptr) {
		curr_pipe_client->disconnectFromServer();
	}

	thread()->quit();
}


//private slots
void
APIServerWorker::OnNewConnection() {
	QLocalSocket *tmp_pipe_client = pipe_server.nextPendingConnection();
	//connect(tmp_pipe_client, &QLocalSocket::disconnected, tmp_pipe_client, &QLocalSocket::deleteLater);		
	connect(tmp_pipe_client, &QLocalSocket::disconnected, [tmp_pipe_client, this] { OnClientDisconnected(tmp_pipe_client); });
	Logger::Log("New pipe client connected", LogEntry::LT_APISERVER);

	if (curr_pipe_client != nullptr) {
		curr_pipe_client->disconnectFromServer();
	}

	curr_pipe_client = tmp_pipe_client;
	connect(curr_pipe_client, &QLocalSocket::readyRead, this, &APIServerWorker::OnClientReadyRead);
}

void 
APIServerWorker::OnClientReadyRead() {
	QString tmp_log_str = "New payload recved";
	
	QByteArray payload = curr_pipe_client->readAll();

	if (payload.size() < 500) {
		tmp_log_str.append(": " % QString(payload));
	}
	else {
		tmp_log_str.append(", data too long to log");
	}

	Logger::Log(tmp_log_str, LogEntry::LT_APISERVER);

	ProcessPayload(payload);
}

void 
APIServerWorker::OnClientDisconnected(QLocalSocket* socket) {
	socket->deleteLater();	//free up object memory next time enters event loop
	if (curr_pipe_client == socket) {
		curr_pipe_client = nullptr;
	}
}

void 
APIServerWorker::ProcessPayload(const QByteArray& payload) {
	QJsonObject json;
	QJsonValue result;
	QList<QVariant> args;

	{
		QJsonDocument json_doc = QJsonDocument::fromJson(payload);
		if (json_doc.isNull()) {
			Logger::Log("Error parsing payload into json", LogEntry::LT_APISERVER);
			goto send_error;
		}

		if (!json_doc.isObject()) {
			Logger::Log("Error payload is not a json object", LogEntry::LT_APISERVER);
			goto send_error;
		}

		json = json_doc.object();
	}

	if (!json.contains("cmd")) {
		Logger::Log("Error payload doesn't contain a command", LogEntry::LT_APISERVER);
		goto send_error;
	}

	APICommand cmd = static_cast<APICommand>(json.value("cmd").toInt(0));	//default to 0 (ERROR) if can't be parsed to int
	Logger::Log("Recv command: " % GetCommandString(cmd), LogEntry::LT_APISERVER);

	if (json.contains("args")) {
		args = json.value("args").toArray().toVariantList();
	}

	switch (cmd) {
	case APICommand::CMD_GETALLTAG:
		result = GetAllTag();
		break;
	case APICommand::CMD_GETTAG:
		break;
	case APICommand::CMD_ADDTAG:
		result = AddTag(args[0].toString());
		break;
	case APICommand::CMD_UPDATETAGNAME:
		result = UpdateTagName(args[0].toUInt(), args[1].toString());
		break;
	case APICommand::CMD_DELETETAG:
		result = RemoveTag(args[0].toUInt());
		break;
	case APICommand::CMD_GETMEDIA:
		if (args.length() > 1)
			result = GetMedia(static_cast<GetMediaType>(args[0].toInt()), args[1]);
		else {
			result = GetMedia(static_cast<GetMediaType>(args[0].toInt()));
		}
		break;
	case APICommand::CMD_GETROOTDIR:
		result = GetRootDir();
		break;
	default:
		Logger::Log("Unknown request command", LogEntry::LT_APISERVER);
		goto send_error;
	}

	curr_pipe_client->write(FormResponse(APICommand::CMD_OK, result));
	curr_pipe_client->flush();
	return;

send_error:

	curr_pipe_client->write(FormResponse(APICommand::CMD_ERROR));
	curr_pipe_client->flush();
}

QByteArray 
APIServerWorker::FormResponse(const APICommand cmd, const QJsonValue& result /* = QJsonObject()*/) {	
	QJsonObject res_object;
	res_object.insert( "cmd", QJsonValue(static_cast<int>(cmd)) );

	/*
	switch (cmd) {
	case APICommand::CMD_ERROR:
		break;
	case APICommand::CMD_OK:
		break;
	case APICommand::CMD_READY:
		break;
	}
	*/

	if (!result.isNull()) {		//defult constructor is null QJsonValue
		res_object.insert("res", result);
	}

	return QJsonDocument(res_object).toJson(QJsonDocument::Compact);
}

QJsonValue
APIServerWorker::GetAllTag() {
	QJsonArray result;

	QVector<ModelTag> m_tag_vec = daemon->GetAllTags();

	for (auto iter = m_tag_vec.cbegin(); iter != m_tag_vec.cend(); iter++) {
		QJsonObject tag_json;

		tag_json.insert("id", (qint64) iter->id);
		tag_json.insert("name", iter->name);
		tag_json.insert("media_count", (qint64) iter->media_count);

		result.push_back(tag_json);
	}

	return result;
}
QJsonValue 
APIServerWorker::GetTag(const unsigned int tag_id) {
	return QJsonValue();
}

QJsonValue 
APIServerWorker::AddTag(const QString& tag_name) {

	unsigned int new_tag_id;
	int ret = daemon->AddTag(tag_name, &new_tag_id);
	if (ret < 0) {
		//TODO: error
	}

	QJsonObject result;

	result.insert("id", (qint64) new_tag_id);
	result.insert("name", tag_name);
	result.insert("media_count", 0);

	return result;
}

QJsonValue 
APIServerWorker::RemoveTag(const unsigned int tag_id) {

	int ret = daemon->RemoveTagById(tag_id);
	if (ret < 0) {
		//TODO:: error
	}

	return QJsonValue();	//default construction is null
}

QJsonValue 
APIServerWorker::UpdateTagName(const unsigned int tag_id, const QString& new_tag_name) {

	int ret = daemon->UpdateTagName(tag_id, new_tag_name);
	if (ret < 0) {
		//TODO: error
	}

	return QJsonValue();
}

QJsonValue
APIServerWorker::GetMedia(GetMediaType type, const QVariant& arg /* = QVariant() */) {

	MediaModelResult mm_res;
	switch (type) {
	case GetMediaType::TYPE_ALL:
		mm_res = daemon->GetAllMedia();
		break;
	case GetMediaType::TYPE_NOTAG:
		mm_res = daemon->GetAllTaglessMedia();
		break;
	case GetMediaType::TYPE_TAG:
		mm_res = daemon->GetTagMedias(arg.toUInt());
		break;
	case GetMediaType::TYPE_QUERY:
		mm_res = daemon->GetQueryMedia(arg.toString());
		break;
	}

	QJsonArray result;

	for (const ModelMedia& m_media: mm_res.model_media_list) {
		QJsonObject media_json;

		media_json.insert("id", (qint64) m_media.id);
		media_json.insert("name", m_media.name);
		media_json.insert("hash", m_media.hash);
		media_json.insert("subdir", m_media.sub_path);

		result.push_back(media_json);
	}

	return result;

}

QJsonValue
APIServerWorker::GetRootDir() {
	return QJsonValue(daemon->GetRootDirectory());
}