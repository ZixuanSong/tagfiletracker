#include "medialistview.h"
#include <QKeyEvent>

MediaListView::MediaListView(QWidget *parent):
	QListView(parent)
{
}


MediaListView::~MediaListView()
{
}

//public 

void
MediaListView::keyPressEvent(QKeyEvent *event) {
	
	if (event->matches(QKeySequence::Copy)) {
		//consume copy shortcut
		event->accept();
		emit CopyShortcut();
	}
	else {
		QListView::keyPressEvent(event);
	}
}

void 
MediaListView::SetConfig (const MediaListViewConfig& config) {
	this->config = config;

	setGridSize(QSize(config.grid_size.w, config.grid_size.h));		//image grid size
	setSpacing(config.spacing);										//spacing between images
}