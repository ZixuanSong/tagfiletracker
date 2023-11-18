#include <QtWidgets/QApplication>

#include "mainUI.h"
#include "daemon.h"
#include "config.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	Logger* logger = Logger::GetInstancePtr();	//make sure logger lives on main thread to recv signals from timer
	qDebug() << "Logger thead affinity: " << logger->thread();

	Daemon daemon;

	Config config;	//TODO: consider singleton	

	if (config.Load() < 0) { 	//loading it early... logs still come through to main ui event loop when it starts up
		Logger::Log("Error loading config file. Using default values...", LogEntry::LT_WARNING);
	}
	
	mainUI main_window(&config, logger, &daemon);
	main_window.show();

	//register custom types
	qRegisterMetaType<ModelTag>();
	qRegisterMetaType<ModelMedia>();
	qRegisterMetaType<LogEntry::LogType>(); 
	qRegisterMetaType<LogEntry>();

	//start daemon thread
	daemon.start();

	return app.exec();
}
