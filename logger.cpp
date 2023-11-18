#include "logger.h"
#include <QStringBuilder>
#include <QDir>
#include <QDateTime>
#include <QDebug>

Logger::Logger()
{
	if (! ( QDir(LOG_DIR_NAME).exists() ) ) {
		QDir::current().mkdir(LOG_DIR_NAME);
	}

	QDateTime date_time = QDateTime::currentDateTime();

	QString file_name = date_time.date().toString("MM-dd-yyyy") % " " % date_time.time().toString("hh-mm-ss");

	log_file.setFileName(QString(LOG_DIR_NAME) % "/" % file_name % ".txt");

	log_file.open(QIODevice::WriteOnly | QIODevice::Text);
	//TODO: what if open fails? no way to report error without using exceptions. message box perhaps?

	anti_spam_timer.setSingleShot(true);				//only pulse once when needed no need to keep pulsing with no log is being pushed
	connect(&anti_spam_timer, &QTimer::timeout, this, &Logger::OnAntiSpamTimerTimeout);

	connect(this, &Logger::StartTimer, this, &Logger::OnStartTimer);	//this convoluted mess is because StartTimer signal could be sent from any thread but OnStartTimer's 
																		//code time->start() must be called from the same thread as GUI because event loop so hook up
																		//to a slot to make sure it always gets called in the same thread as logger's affinity thread
}


Logger::~Logger()
{
	if (log_file.isOpen()) {
		log_file.flush();
		log_file.close();
	}
}


//static entry point callable from every subsystem
void
Logger::Log(const QString& log_str, LogEntry::LogType type) {
	GetInstancePtr()->LogEmitSignal(log_str, type);
}


Logger*
Logger::GetInstancePtr() {

	//automatically locked at initialization
	//TODO: think about overhead of static possible to avoid?
	static Logger singleton_logger;
	return &singleton_logger;
}


int
Logger::LogEmitSignal(const QString& log_text, LogEntry::LogType log_type) {
	
	QString full_text = QDateTime::currentDateTime().time().toString("hh:mm:ss") % "  " % log_text;

	lock.lock();

	log_file.write(full_text.toUtf8());
	log_file.write("\n");
	log_file.flush();			//degrades performance but what's the point of log if the contents are not written to file (buffered) when program crashes

	log_entry_vec.push_back( LogEntry(log_type, std::move(full_text)) );	//move full text it has no use at this point
	
	lock.unlock();
	
	emit StartTimer(GetSpamTimerTimeoutMsec());	//could be direct or queued connection to the handler
	
	return 1;
}

//private slot

//logger created in main thread, this signal is called from main thread
void 
Logger::OnAntiSpamTimerTimeout() {

	lock.lock();

	if (log_entry_vec.size() > 1) {
		emit NewLogEntries(log_entry_vec);
	} else if (log_entry_vec.size() == 1) {
		emit NewLogEntry(log_entry_vec[0]);
	}

	log_entry_vec.clear();

	lock.unlock();
}

void 
Logger::OnStartTimer(int msec) {

	if (!anti_spam_timer.isActive()) {		//if queued connection need to check to make sure timer isn't being accidently restarted by another signal
		anti_spam_timer.start(std::chrono::milliseconds(msec));
	}
}

void	
Logger::SetSpamTimerTimeoutMsec (int msec) {
	config_lock.lock();
	spam_timer_timeout_msec = msec;
	config_lock.unlock();
}

int		
Logger::GetSpamTimerTimeoutMsec () {
	config_lock.lock();
	int msec = spam_timer_timeout_msec;
	config_lock.unlock();
	return msec;
}