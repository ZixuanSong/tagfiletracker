#pragma once

#include <QListView>

#include "config.h"

class MediaListView: public QListView
{

	Q_OBJECT

public:
	MediaListView(QWidget *parent = nullptr);
	~MediaListView();

	void keyPressEvent(QKeyEvent *event) override;

	void SetConfig(const MediaListViewConfig& config);

signals:
	void CopyShortcut();

private:

	MediaListViewConfig config;
};

