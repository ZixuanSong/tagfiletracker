#pragma once

#include <QWidget>
#include "ui_LogWindow.h"

class LogWindow : public QWidget
{
	Q_OBJECT

public:
	LogWindow(QWidget *parent = Q_NULLPTR);
	~LogWindow();

private:
	Ui::LogWindow ui;
};
