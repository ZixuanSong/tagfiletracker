#pragma once

#include <QtWidgets/QMainWindow>
#include <QSortFilterProxyModel>
#include <QCompleter>
#include <QShortcut>
#include "ui_mainUI.h"
#include "TagModel.h"
#include "LogModel.h"
#include "MediaModel.h"
#include "MediaTagModel.h"
#include "logger.h"
#include "daemon.h"
#include "LogWindow.h"
#include "config.h"
#include "api_server.h"

#define THUMBNAIL_CACHE_MAX_ITEM_CAP	100	//TODO: terrible, make it configurable in the future

/*
	Design decision:

	1. Who should handle signals emited by view widgets (ex. doubleClicked())?
	Mainwindow should handle these events. Although it might be more efficient for the underlying
	models to handles these events, sometimes an intermediate processing is required for the 
	correct behavior. In case of clicking on proxy models, main window should map the proxy index
	to the source index then pass the control onto models. May consider connecting signals that
	don't require intermediate processing directly to model slots in the future. However, mainwindow
	should never attempt to run daemon functions directly. Those logic are reservered for 
	underlying models. Mainwindow only act as a glue between widgets by passing values.
*/

class mainUI : public QMainWindow
{
	Q_OBJECT

public:

	mainUI(Config* config, Logger* logger, Daemon* daemon, QWidget *parent = Q_NULLPTR) ;

public slots:
	
	//UI slots

	//buttons
	void OnNewTagPushButtonRelease();

	void OnDeleteTagPushButtonRelease();

	void OnAllMediaPushButtonRelease();

	void OnAllTaglessMediaPushButtonRelease();

	void OnClearMediaPushButtonRelease();

	void OnAddMediaTagPushButtonRelease();

	void OnQueryPushButtonRelease();

	void OnGenerateHashButtonRelease();

	//media tag
	void OnMediaTagViewContextMenuRequest(const QPoint&);	//right clicking on media tag list view

	void OnMediaTagContextMenuDeleteTriggered();			//clicked delete option from right clicking on media tag list view

	void OnMediaTagSearchLineEditEnterPressed();

	void OnMediaTagSet(const ModelMedia&);					//media tag view has been set to a new media
	
	void OnMediaTagReset();									//media tag view has be reset

	//tag view

	void OnTagViewClicked(const QModelIndex&);				//clicked on a tag from tag list view

	//media view

	void OnMediaViewDoubleClicked(const QModelIndex&);		//double clicked on a media from media view
	
	void OnMediaViewCopyShortcut();							//ctrl+C on a media in media list view

	void OnMediaVerticleScrollBar();						//media list view scroll position changed

	void OnMediaViewSelectionCurrentIndexChanged(const QModelIndex&, const QModelIndex&);

	//query line edit

	void OnQueryLineEditReturnPressed();					//pressed enter on query line edit

	//daemon event slots

	void OnDaemonInitialized();

	void OnDaemonTagInserted(const ModelTag&);

	void OnDaemonTagNameUpdated(const unsigned int, const QString&);

	void OnDaemonTagRemoved(const unsigned int);

	void OnDaemonTaglessMediaInserted(const ModelMedia&);

	void OnDaemonMediaInserted(const ModelMedia&);	//when will this actually happen? new medias are always tagless

	void OnDaemonMediaNameUpdated(const unsigned int, const QString&);

	void OnDaemonMediaSubdirUpdated(const unsigned int, const QString&);

	void OnDaemonMediaRemoved(const unsigned int);

	void OnDaemonLinkFormed(const ModelTag&, const unsigned int);

	void OnDaemonLinkDestroyed(const unsigned int, const unsigned int);

	//shortcuts
	
	void OnFindShortcutActivated();

	void OnMediaTagLineEditShortcutActivated();

private:
	Ui::mainUIClass ui;

	Daemon*		daemon;
	Config*		config;
	
	APIServer				api_server;
	QCompleter				media_tag_tag_name_completer;
	QCompleter				query_tag_name_completer;

	QSortFilterProxyModel	tag_filter_prox_model;

	TagModel				tag_model;
	MediaModel				media_model;
	MediaTagModel			media_tag_model;
	LogModel				log_model;
	ThumbnailProvider		thumbnail_provider;

	QShortcut				query_global_shortcut;
	QShortcut				media_tag_line_edit_shortcut;

	void	UpdateThumbnailGenerationQueue();
};
