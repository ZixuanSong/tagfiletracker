#pragma once

#include <deque>

#include <QString>
#include <QObject>
#include <QMutex>
#include <QMetaType>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QVector>
#include <memory>

#define	LOG_DIR_NAME	"logs"

struct LogEntry {

	enum LogType {
		LT_NORMAL,		//no color
		LT_WARNING,		//yellow
		LT_ERROR,		//red
		LT_SUCCESS,		//green
		LT_ATTN,		//light blue
		LT_MONITOR,		//light purple
		LT_APISERVER	//orange
	};

	LogType		type;
	QString		content;

	LogEntry() = default;

	LogEntry(LogType type, QString content) :
		type(type),
		content(std::move(content))
	{
	}
};

Q_DECLARE_METATYPE(LogEntry::LogType);
Q_DECLARE_METATYPE(LogEntry);


//singleton object

class Logger : public QObject{

	Q_OBJECT

public:

	Logger(const Logger& l) = delete;			//no copy constructor
	void operator=(const Logger& l) = delete;	//no copy operator

	static void Log(const QString& log_str, LogEntry::LogType type = LogEntry::LT_NORMAL);			//the interface any class can access

	static Logger* GetInstancePtr();

	int LogEmitSignal(const QString& log_str, LogEntry::LogType type = LogEntry::LT_NORMAL);		//has to make a member function in order to emit signal

	void	SetSpamTimerTimeoutMsec(int msec);
	int		GetSpamTimerTimeoutMsec();

signals:

	void NewLogEntry(const LogEntry& entry);					//these two signals can pass const ref because logger obeject will live on same thread
	void NewLogEntries(const QVector<LogEntry>& entry_vec);		//as logmodel object, connection will be direct.
	void StartTimer(int msec);

private slots:

	void OnAntiSpamTimerTimeout();
	void OnStartTimer(int msec);

private:

	Logger();
	~Logger();

	QMutex								config_lock;
	int									spam_timer_timeout_msec;

	QTimer								anti_spam_timer;			//prevent logger from spamming single signals to the UI, instead 
																	//send multiples at once
	QFile								log_file;
	QMutex								lock;
	QVector<LogEntry>					log_entry_vec;
};

