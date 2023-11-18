#include "LogWindow.h"

LogWindow::LogWindow(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	QWidget::setWindowFlags(Qt::Window);
}

LogWindow::~LogWindow()
{
}
