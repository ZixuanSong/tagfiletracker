#pragma once

#include <QThread>
#include <QVector>
#include <QLocalSocket>
#include <QLocalServer>
#include <QMetaObject>
#include <QByteArray>
#include <QJsonArray>

#include "logger.h"
#include "daemon.h"

#define THREAD_QUIT_WAIT_MSEC 3000

/*
	Each pipe transaction is a compact JSON object encoded as utf8 string

	General structure:
	{
		cmd: <APICommand>,

		...extra payload properties
	}

	Client request structure:
	{
		cmd: <APICommand>,
		args: []
	}

	Server response structure:
	{
		cmd: <APICommand>,
		res: {
			
			... //command specific properties
		}
	}

*/



#define PIPE_NAME "TAGSEARCH_PIPE"

enum class APICommand {
	CMD_ERROR,
	CMD_OK,
	CMD_READY,
	CMD_GETALLTAG,
	CMD_GETTAG,
	CMD_ADDTAG,
	CMD_UPDATETAGNAME,
	CMD_DELETETAG,
	CMD_GETMEDIA,
	CMD_GETROOTDIR
};

enum class GetMediaType {
	TYPE_ALL,
	TYPE_NOTAG,
	TYPE_TAG,
	TYPE_QUERY
};

//Qt requires worker-object approach in order to have slots be run in a new thread's event loop
class APIServerWorker: public QObject {
	Q_OBJECT

public:

	APIServerWorker(Daemon* daemon);
	APIServerWorker();

	APIServerWorker(const APIServerWorker&) = delete;
	APIServerWorker& operator= (const APIServerWorker&) = delete;

	static QString GetCommandString(const APICommand cmd);

public slots:

	void WorkerMain();	//first handler/slot to be called on the server thread
	void OnCleanup();	//called right before terminating thread event loop

private slots:
	
	void OnNewConnection();
	void OnClientReadyRead();
	void OnClientDisconnected(QLocalSocket* socket);

private:

	Daemon*						daemon;

	QLocalServer				pipe_server;
	//QMetaObject::Connection		client_readyready_connection;
	QLocalSocket*				curr_pipe_client = nullptr;

	QVector<QLocalSocket*>		accepted_connections;

	void ProcessPayload(const QByteArray& payload);
	QByteArray FormResponse(const APICommand cmd, const QJsonValue& result = QJsonValue());


	//command handlers
	QJsonValue GetAllTag();
	QJsonValue GetTag(const unsigned int tag_id);
	QJsonValue AddTag(const QString& tag_name);
	QJsonValue RemoveTag(const unsigned int tag_id);
	QJsonValue UpdateTagName(const unsigned int tag_id, const QString& new_tag_name);
	QJsonValue GetMedia(GetMediaType type, const QVariant& arg = QVariant());
	QJsonValue GetRootDir();

};


class APIServer: public QObject {
	Q_OBJECT

public:
	APIServer(Daemon* daemon):
		worker(daemon)
	{
		connect(&server_thread, &QThread::started, &worker, &APIServerWorker::WorkerMain);	//once thread started, call worker entry
		connect(this, &APIServer::WorkerCleanup, &worker, &APIServerWorker::OnCleanup);		//tell worker to clean up any resources before stopping thread event loop

		connect(&server_thread, &QThread::finished, this, &APIServer::OnThreadFinished);	//log when thread event loop finishes
	}

	~APIServer() {
		Stop();
	}

	APIServer(const APIServer&) = delete;
	APIServer& operator= (const APIServer&) = delete;

	void Start() {

		worker.moveToThread(&server_thread);
		
		Logger::Log("Starting API Server thread...", LogEntry::LT_ATTN);

		server_thread.start();
	}

	void Stop() {
		emit WorkerCleanup();
		server_thread.wait();
	}

signals:
	void WorkerCleanup();

private slots:

	//called when event loop as stopped
	void OnThreadFinished() {
		Logger::Log("API server thread exited", LogEntry::LT_ATTN);
	}

private:

	APIServerWorker worker;	//impl of server code, worker object required for signal and slot
	QThread server_thread;
};

