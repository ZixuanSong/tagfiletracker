#include "mainUI.h"
#include <QHeaderView>
#include <QtConcurrent/qtconcurrentrun.h>
#include "query.h"
#include <qvector.h>
#include <QSysInfo>
#include <QDir>
#include <QProcess>
#include <QMenu>
#include <QClipboard>
#include <QScrollBar>
#include <QItemSelectionModel>
#include <QPixmap>

mainUI::mainUI(Config* config, Logger* logger, Daemon* daemon, QWidget *parent) :
	QMainWindow(parent),
	daemon(daemon),
	api_server(daemon),
	tag_model(daemon),
	media_model(daemon, &thumbnail_provider),
	media_tag_model(daemon),
	query_global_shortcut(this),
	media_tag_line_edit_shortcut(this),
	thumbnail_provider(config->GetEffectiveSubconfig(Config::THUMBGEN).thumbnail_gen_config)
{

	api_server.Start();

	ui.setupUi(this);

	/*
		Logger set up
	*/

	logger->SetSpamTimerTimeoutMsec(config->GetEffectiveSubconfig(Config::LOGGER).logger_config.anti_spam_timeout_msec);

	/*
		Main window set up
	*/

	//statusBar()->setSizeGripEnabled(false);									//disable size grip

	connect(ui.new_tag_push_button, SIGNAL(released()), this, SLOT(OnNewTagPushButtonRelease()));									//handle new tag button
	connect(ui.delete_tag_push_button, SIGNAL(released()), this, SLOT(OnDeleteTagPushButtonRelease()));								//handle delete tag button
	connect(ui.all_media_push_button, SIGNAL(released()), this, SLOT(OnAllMediaPushButtonRelease()));								//handle all media button
	connect(ui.tagless_media_push_button, SIGNAL(released()), this, SLOT(OnAllTaglessMediaPushButtonRelease()));					//handle all tagless media button
	connect(ui.clear_media_push_button, SIGNAL(released()), this, SLOT(OnClearMediaPushButtonRelease()));							//handle clear media button
	connect(ui.query_push_button, &QPushButton::released, this, &mainUI::OnQueryPushButtonRelease);					//handle query pushbutton
	connect(ui.media_tag_list_view, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnMediaTagViewContextMenuRequest(const QPoint&)));	//handle media tag list view context menu request

	//connect daemon signals
	connect(daemon, &Daemon::Initialized, this, &mainUI::OnDaemonInitialized);
	
	/*
		Log list view/model setup
	*/

	ui.log_list_view->setModel(&log_model);					//set log model
	
	log_model.SetMaxCacheSize(config->GetEffectiveSubconfig(Config::LOGGER).logger_config.cache_size);
	connect(logger, &Logger::NewLogEntry, &log_model, &LogModel::OnNewLogEntry);											//inform log model of new item
	connect(logger, &Logger::NewLogEntries, &log_model, &LogModel::OnNewLogEntries);										//inform log model of new items
	connect(&log_model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), ui.log_list_view, SLOT(scrollToBottom()));		//scroll to bottom when new log is inserted

	/*
		Tag table view/model setup
	*/

	tag_filter_prox_model.setSourceModel(&tag_model);							//set source model for sorting

	ui.tag_table_view->setModel(&tag_filter_prox_model);						//set tag proxy model
	ui.tag_table_view->verticalHeader()->setVisible(false);						//hide row header
	ui.tag_table_view->setSelectionBehavior(QAbstractItemView::SelectRows);		//select whole row only not individual cell
	ui.tag_table_view->setSelectionMode(QAbstractItemView::SingleSelection);	//can only select one thing at a time
	ui.tag_table_view->setSortingEnabled(true);									//add the ability to sort by col

	connect(ui.tag_table_view, &QTableView::clicked, this, &mainUI::OnTagViewClicked);	//handle double on tag table view

	/*
		Media view/model set up
	*/

	ui.media_list_view->SetConfig(config->GetEffectiveSubconfig(Config::MEDIAVIEW).media_list_view_config);

	ui.media_list_view->setViewMode(QListView::IconMode);		//display list of images
	ui.media_list_view->setSelectionMode(QAbstractItemView::SingleSelection);	//can only select one thing at a time
	ui.media_list_view->setModel(&media_model);					//set model
	//ui.media_list_view->setIconSize(QSize(140, 140));			//set icon size
	//ui.media_list_view->setLayoutMode(QListView::Batched);		//batch process
	//ui.media_list_view->setUniformItemSizes(true);				//uniform size
	ui.media_list_view->setResizeMode(QListView::Adjust);		//adjust images when window is resized

	connect(&thumbnail_provider, &ThumbnailProvider::NewThumbnail, &media_model, &MediaModel::OnMediaToIconComplete);
	connect(ui.media_list_view, &MediaListView::doubleClicked, this, &mainUI::OnMediaViewDoubleClicked);
	connect(ui.media_list_view, &MediaListView::CopyShortcut, this, &mainUI::OnMediaViewCopyShortcut);
	connect(ui.media_list_view->verticalScrollBar(), &QScrollBar::valueChanged, this, &mainUI::OnMediaVerticleScrollBar);
	connect(ui.media_list_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &mainUI::OnMediaViewSelectionCurrentIndexChanged);

	/*
		Media info group related
	*/

	ui.media_info_thumb_label->setAlignment(Qt::AlignCenter);

	ui.media_info_name_line_edit->setReadOnly(true);	//name line edit	
	ui.media_info_subdir_line_edit->setReadOnly(true);	//subdir line edit
	ui.media_info_hash_line_edit->setReadOnly(true);	//hash line edit

	connect(ui.media_info_hash_regen_push_button, &QPushButton::released, this, &mainUI::OnGenerateHashButtonRelease);

	//meida tag list view
	ui.media_tag_list_view->setModel(&media_tag_model);						//set model
	ui.media_tag_list_view->setContextMenuPolicy(Qt::CustomContextMenu);	//allow right click context menu

	//add media tag push button
	connect(ui.add_media_tag_push_button, &QPushButton::released, this, &mainUI::OnAddMediaTagPushButtonRelease);						//handle add media tag button

	//media tag line edit
	connect(ui.media_tag_search_line_edit, &QLineEdit::returnPressed, this, &mainUI::OnMediaTagSearchLineEditEnterPressed);			//pressing enter is same as pushing add tag button'			

	//media info related
	connect(&media_tag_model, &MediaTagModel::MediaSet, this, &mainUI::OnMediaTagSet);
	connect(&media_tag_model, &MediaTagModel::MediaReset, this, &mainUI::OnMediaTagReset);

	//tag name search completer
	media_tag_tag_name_completer.setModel(&tag_model);							//set completer model
	media_tag_tag_name_completer.setCompletionColumn(0);							//1st col (name) as completion
	media_tag_tag_name_completer.setFilterMode(Qt::MatchContains);				//doesnt necessary match from beginning
	media_tag_tag_name_completer.setMaxVisibleItems(10);							//10 possible entries show up at most

	ui.media_tag_search_line_edit->SetCompleter(&media_tag_tag_name_completer);	//set completer
	ui.media_tag_search_line_edit->AddSeparator(',');

	/*
		Query line edit
	*/

	connect(ui.query_line_edit, &QLineEdit::returnPressed, this, &mainUI::OnQueryLineEditReturnPressed);
	//tag name search completer
	query_tag_name_completer.setModel(&tag_model);									//set completer model
	query_tag_name_completer.setCompletionColumn(0);								//1st col (name) as completion
	query_tag_name_completer.setFilterMode(Qt::MatchContains);						//doesnt necessary match from beginning
	query_tag_name_completer.setMaxVisibleItems(20);								//20 possible entries show up at most

	ui.query_line_edit->SetCompleter(&query_tag_name_completer);					//set completer
	ui.query_line_edit->AddSeparatorList(QList<QChar>{'+', '-', '*', '(', ')'});	//query operators are separators

	/*
		Log window related
	*/

	ui.log_docked_widget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);		//docked widget should not be closable, log is always present


	/*
		Global Shortcuts
	*/

	query_global_shortcut.setKey(QKeySequence(QKeySequence::Find));
	media_tag_line_edit_shortcut.setKey(QKeySequence(QKeySequence::New));
}

/*
	Button slots
*/

void
mainUI::OnNewTagPushButtonRelease() {
	QString new_tag_name = ui.tag_search_line_edit->text();
	new_tag_name = new_tag_name.trimmed();

	if (new_tag_name.size() == 0) {
		Logger::Log("Tag name empty", LogEntry::LT_WARNING);
		return;
	}

	//make sure tag name doesnt include ' or " as it's reserved in query
	for (QChar ch : new_tag_name) {
		if (ch == '\'' || ch == '\"') {
			Logger::Log("Tag name must not contain quotes ( \' or \" )", LogEntry::LT_ERROR);
			return;
		}
	}

	tag_model.DaemonAddTag(new_tag_name);
}

void 
mainUI::OnDeleteTagPushButtonRelease() {

	//nothing is selected on tag view
	if (!ui.tag_table_view->selectionModel()->hasSelection()) {
		return;
	}

	QModelIndexList index_list = ui.tag_table_view->selectionModel()->selectedRows();
	
	//something is wrong... only 1 row is alloed to be selected at a time
	if (index_list.size() != 1) {
		return;
	}

	QModelIndex source_idx = tag_filter_prox_model.mapToSource(index_list.front());

	tag_model.DaemonRemoveTagByIndex(source_idx);
}

void
mainUI::OnAllMediaPushButtonRelease() {
	media_model.UpdateToAllMedia();

}

void 
mainUI::OnAllTaglessMediaPushButtonRelease() {
	media_model.UpdateToAllTaglessMedia();

}

void
mainUI::OnClearMediaPushButtonRelease() {
	media_model.Reset();
}

void
mainUI::OnAddMediaTagPushButtonRelease() {

	//no media selected
	if (!media_tag_model.HasMedia()) {
		return;
	}

	QStringList tag_name_list = ui.media_tag_search_line_edit->text().split(',', QString::SkipEmptyParts);
	for (QString tag_name : tag_name_list) {
		tag_name = tag_name.trimmed();
		//TODO: user input sanitation
		media_tag_model.AddMediaTagByName(tag_name);
	}
}

void
mainUI::OnQueryPushButtonRelease() {

	OnQueryLineEditReturnPressed();
}

/*
	Media tag view slots
*/

void 
mainUI::OnMediaTagViewContextMenuRequest(const QPoint& widget_pos) {

	//do not display menu if cursor is not on an item
	if (!ui.media_tag_list_view->indexAt(widget_pos).isValid()) {
		return;
	}

	QMenu media_tag_item_context_menu;

	media_tag_item_context_menu.addAction("Delete", this, SLOT(OnMediaTagContextMenuDeleteTriggered()));

	media_tag_item_context_menu.exec(ui.media_tag_list_view->mapToGlobal(widget_pos));
}

void
mainUI::OnMediaTagContextMenuDeleteTriggered() {

	unsigned int selected_row_id = ui.media_tag_list_view->selectionModel()->selectedIndexes().front().row();
	media_tag_model.UnlinkMediaTagByRow(selected_row_id);
}

void 
mainUI::OnMediaTagSearchLineEditEnterPressed() {

	if (media_tag_tag_name_completer.popup()->isVisible()) {
		//the completer pop up is showing, user might press enter, eat the linedit enter signal
		return;
	}

	OnAddMediaTagPushButtonRelease();
}

void
mainUI::OnMediaTagSet(const ModelMedia& m_media) {

	QPixmap p;
	p.convertFromImage(thumbnail_provider.GetMediaThumbnail(m_media.id, m_media.GetAbsPath()));
	ui.media_info_thumb_label->setPixmap(p);

	ui.media_info_id_label->setText(QString::number(m_media.id));
	ui.media_info_name_line_edit->setText(m_media.name);
	ui.media_info_subdir_line_edit->setText(m_media.sub_path);
	ui.media_info_hash_line_edit->setText(m_media.hash);

	ui.media_info_name_line_edit->home(false);			//sets the cursor to be beginning in case the text is too long and cursor scrolls to the end
	ui.media_info_subdir_line_edit->home(false);		
	ui.media_info_hash_line_edit->home(false);

	//ui.media_info_hash_regen_push_button->setEnabled(true);

	ui.media_tag_search_line_edit->setEnabled(true);
	ui.add_media_tag_push_button->setEnabled(true);
}

void
mainUI::OnMediaTagReset() {
	ui.media_info_id_label->setText("");
	ui.media_info_name_line_edit->setText("");
	ui.media_info_hash_line_edit->setText("");

	ui.media_tag_search_line_edit->setEnabled(false);
	ui.media_info_hash_regen_push_button->setEnabled(false);
	ui.add_media_tag_push_button->setEnabled(false);
}

void 
mainUI::OnGenerateHashButtonRelease() {
	LogWindow *log = new LogWindow(this);
	log->show();
}

/*
	Tag view slots
*/

void
mainUI::OnTagViewClicked(const QModelIndex& index) {

	//conver to model index from proxy
	unsigned int tag_id;
	tag_model.GetTagIdByIndex(tag_filter_prox_model.mapToSource(index), &tag_id);

	if (QGuiApplication::keyboardModifiers() != Qt::ControlModifier) {
		media_model.UpdateByTagId(tag_id);
		return;
	}
	
	if (!media_tag_model.HasMedia()) {
		return;
	}

	media_tag_model.AddMediaTagById(tag_id);
}

/*
	Media view slots
*/

void 
mainUI::OnMediaViewDoubleClicked(const QModelIndex& index) {
	//open media file location

	//windows support only so far
	if (QSysInfo::productType() != "windows") {
		return;
	}

	QString full_path;
	media_model.GetMediaFullPathByIndex(index, &full_path);

	//stole from stack overflow
	QStringList args;
	args << "/select," << QDir::toNativeSeparators(full_path);
	QProcess::startDetached("explorer", args);
}

void
mainUI::OnMediaViewCopyShortcut() {
	
	QModelIndex index = ui.media_list_view->currentIndex();
	if (index.isValid()) {
		QString full_path;
		media_model.GetMediaFullPathByIndex(index, &full_path);
		QApplication::clipboard()->setText(full_path);
	}
}

void
mainUI::OnMediaVerticleScrollBar() {
	UpdateThumbnailGenerationQueue();
}

void 
mainUI::OnMediaViewSelectionCurrentIndexChanged(const QModelIndex& curr, const QModelIndex& prev) {

	ui.media_info_hash_regen_push_button->setEnabled(false);
	
	if (curr.isValid()) {

		ModelMedia buff;

		media_model.GetModelMediaByIndex(curr, &buff);

		setWindowTitle(buff.GetAbsPath());

		media_tag_model.SetMedia(buff);
	}
	else {

		media_tag_model.Reset();
	}
}

/*
	Query line edit slots
*/

void
mainUI::OnQueryLineEditReturnPressed() {
	//trim query text
	QString query_str = ui.query_line_edit->text().trimmed();

	if (query_str.isEmpty()) {
		Logger::Log("Query empty!", LogEntry::LT_WARNING);
		return;
	}

	media_model.UpdateByQuery(query_str);
}

/*
	Daemon slots
*/

void 
mainUI::OnDaemonInitialized() {

	tag_model.GetAllTags();

	//connect daemon signals
	connect(daemon, &Daemon::TagInserted, this, &mainUI::OnDaemonTagInserted);						//new tag
	connect(daemon, &Daemon::TagRemoved, this, &mainUI::OnDaemonTagRemoved);						//tag removed
	connect(daemon, &Daemon::TagNameUpdated, this, &mainUI::OnDaemonTagNameUpdated);				//tag updated

	connect(daemon, &Daemon::TaglessMediaInserted, this, &mainUI::OnDaemonTaglessMediaInserted);	//media becomes tagless
	connect(daemon, &Daemon::MediaNameUpdated, this, &mainUI::OnDaemonMediaNameUpdated);			//media name change
	connect(daemon, &Daemon::MediaSubdirUpdated, this, &mainUI::OnDaemonMediaSubdirUpdated);		//media moved
	connect(daemon, &Daemon::MediaRemoved, this, &mainUI::OnDaemonMediaRemoved);					//media removed

	connect(daemon, &Daemon::LinkFormed, this, &mainUI::OnDaemonLinkFormed);						//link formed
	connect(daemon, &Daemon::LinkDestroyed, this, &mainUI::OnDaemonLinkDestroyed);					//link broken

	ui.all_media_push_button->setEnabled(true);
	ui.clear_media_push_button->setEnabled(true);
	ui.tagless_media_push_button->setEnabled(true);
	ui.query_line_edit->setEnabled(true);
	ui.query_push_button->setEnabled(true);
	ui.new_tag_push_button->setEnabled(true);
	ui.delete_tag_push_button->setEnabled(true);
	ui.tag_search_line_edit->setEnabled(true);

	connect(&query_global_shortcut, &QShortcut::activated, this, &mainUI::OnFindShortcutActivated);							//global shortcut for query
	connect(&media_tag_line_edit_shortcut, &QShortcut::activated, this, &mainUI::OnMediaTagLineEditShortcutActivated);		//global shortcut for media tag add

	qDebug() << "GUI thread: " << this->thread();
}

void 
mainUI::OnDaemonTagInserted(const ModelTag& new_tag) {

	//update the tag model a new tag is inserted
	tag_model.InsertTag(new_tag);
}

void 
mainUI::OnDaemonTagNameUpdated(const unsigned int tag_id, const QString& new_name) {

	//update tag model's modeltag name
	if (tag_model.TagExistById(tag_id)) {
		tag_model.UpdateTagName(tag_id, new_name);
	}

	//update name if tag is media tag list
	if (media_tag_model.TagExist(tag_id)) {
		media_tag_model.UpdateTagName(tag_id, new_name);
	}
}

void 
mainUI::OnDaemonTagRemoved(const unsigned int tag_id) {

	//inform tag model to remove entry 
	if (tag_model.TagExistById(tag_id)) {
		tag_model.RemoveTagById(tag_id);
	}

	//media tag displays this tag we need to remove it
	if (media_tag_model.TagExist(tag_id)) {
		media_tag_model.RemoveMediaTag(tag_id);
	}

	//media is currently querried with this tag we need to clear the media as the result is no longer valid
	if (media_model.GetDisplayMode() == MediaModel::QUERY && media_model.IsAssociatedTagId(tag_id)) {
		media_model.Reset();
	}
}

void
mainUI::OnDaemonTaglessMediaInserted(const ModelMedia& tagless_media) {

	//insert this new tagless media if media model is in tagless mode
	if (media_model.GetDisplayMode() == MediaModel::TAGLESS || media_model.GetDisplayMode() == MediaModel::ALL) {
		media_model.InsertMedia(tagless_media);
	}
}


void 
mainUI::OnDaemonMediaInserted(const ModelMedia& new_media) {

}

void 
mainUI::OnDaemonMediaNameUpdated(const unsigned int media_id, const QString& new_name) {
	//update media model if this media is currently being displayed
	if (media_model.IsMediaInModel(media_id)) {
		media_model.UpdateMediaName(media_id, new_name);
	}
}

void 
mainUI::OnDaemonMediaSubdirUpdated(const unsigned int media_id, const QString& new_subdir) {
	//update media model if this media is currently being displayed
	if (media_model.IsMediaInModel(media_id)) {
		media_model.UpdateMediaSubdir(media_id, new_subdir);
	}
}

void 
mainUI::OnDaemonMediaRemoved(const unsigned int media_id) {
	//remove from media model if this media is currently being displayed
	if (media_model.IsMediaInModel(media_id)) {
		media_model.RemoveMeida(media_id);
	}

	//reset media tag list if this media no longer exist
	if (media_tag_model.HasMedia() && media_tag_model.GetMediaId() == media_id) {
		media_tag_model.Reset();
	}

	thumbnail_provider.RemoveMediaThumbnail(media_id);
}

void 
mainUI::OnDaemonLinkFormed(const ModelTag& tag, const unsigned int media_id) {

	if (media_model.GetDisplayMode() == MediaModel::TAGLESS && media_model.IsMediaInModel(media_id)) {

		//save scroll position
		int prev_pos = ui.media_list_view->verticalScrollBar()->value();

		//this media is no longer tagless, remove it from view
		media_model.RemoveMeida(media_id);

		//restore scroll position
		if (ui.media_list_view->verticalScrollBar()->maximum() < prev_pos) {
			ui.media_list_view->verticalScrollBar()->setValue(ui.media_list_view->verticalScrollBar()->maximum());
		}
		else {
			ui.media_list_view->verticalScrollBar()->setValue(prev_pos);
		}
		

	} else if (media_model.GetDisplayMode() == MediaModel::QUERY && media_model.IsAssociatedTagId(tag.id)) {

		//if the current media display is associated with this tag 
	}
	
	//current media tag model is focused on this media
	if (media_tag_model.HasMedia() && media_tag_model.GetMediaId() == media_id) {
		media_tag_model.InsertMediaTag(tag);
	}

	//increment tag media count
	tag_model.IncTagMediaCount(tag.id);
}

void 
mainUI::OnDaemonLinkDestroyed(const unsigned int tag_id, const unsigned int media_id) {

	//if the current media display is associated with this tag
	if (media_model.GetDisplayMode() == MediaModel::QUERY && media_model.IsAssociatedTagId(tag_id)) {

	}

	//current media tag model is focused on this media
	//deleting model tag from media tag model is handled when right clicked on delete
	//if (media_tag_model.GetMediaId() == media_id) {
		//media_tag_model.RemoveMediaTag(tag_id);
	//}

	//decrement tag media count
	if (tag_model.TagExistById(tag_id)) {
		tag_model.DecTagMediaCount(tag_id);
	}
}

void 
mainUI::OnFindShortcutActivated() {
	ui.query_line_edit->setFocus();
	ui.query_line_edit->selectAll();
}

void 
mainUI::OnMediaTagLineEditShortcutActivated() {
	if (media_tag_model.HasMedia()) {
		ui.media_tag_search_line_edit->setFocus();
		ui.media_tag_search_line_edit->selectAll();
	}
}

//private

void
mainUI::UpdateThumbnailGenerationQueue() {

	QSize grid_size = ui.media_list_view->gridSize();
	QSize viewport_size = ui.media_list_view->viewport()->size();
	QModelIndex begin_idx = ui.media_list_view->indexAt(QPoint(grid_size.width() / 2, 1));
	QModelIndex end_idx = ui.media_list_view->indexAt(QPoint(viewport_size.width() / grid_size.width() * grid_size.width() - (grid_size.width() / 2),
		viewport_size.height() - 1));
	//qDebug() << begin_idx.row() << " , " << end_idx.row();

	QQueue<ModelMedia> new_thumbnail_gen_queue;
	media_model.GetModelMediaListByIndex(begin_idx, end_idx, &new_thumbnail_gen_queue);

	if (new_thumbnail_gen_queue.empty()) {
		return;
	}

	thumbnail_provider.UpdateGenerateQueue(new_thumbnail_gen_queue);
}