#include "multicompleterlineedit.h"
#include <QDebug>
#include <QAbstractItemView>
#include <QApplication>

MultiCompleterLineEdit::MultiCompleterLineEdit(QWidget *parent)
{
	//connect(this, &MultiCompleterLineEdit::textEdited, this, &MultiCompleterLineEdit::OnTextEdited);
	connect(this, &MultiCompleterLineEdit::cursorPositionChanged, this, &MultiCompleterLineEdit::OnCursorPositionChanged);

	cursor_pos_change_ignore_flag = false;
}


MultiCompleterLineEdit::~MultiCompleterLineEdit()
{
}

void 
MultiCompleterLineEdit::SetCompleter(QCompleter* completer) {
	this->completer = completer;
	this->completer->setWidget(this);

	connect(completer, qOverload<const QString&>(&QCompleter::highlighted), this, &MultiCompleterLineEdit::OnCompleterHighlighted);
}

void 
MultiCompleterLineEdit::AddSeparator(const QChar separator) {
	if (!separator_set.contains(separator)) {
		separator_set.insert(separator);
	}
}

void 
MultiCompleterLineEdit::AddSeparatorList(const QList<QChar>& sep_list) {
	for (auto iter = sep_list.begin(); iter != sep_list.end(); iter++) {
		AddSeparator(*iter);
	}
}

//private slots:

void 
MultiCompleterLineEdit::OnTextEdited(const QString& new_text) {

	//obviously nothing goes on if there is no text
	if (new_text.size() == 0) {
		return;
	}

	UpdateCurrentSegment();

	QString segment;
	GetCurrentSegmentText(&segment);

	qDebug() << '(' << segment << ')';
}

void
MultiCompleterLineEdit::OnCompleterHighlighted(const QString& highlighted_text) {
	
	QString curr_text = text();
	curr_text.replace(curr_segment_text_start_idx, curr_segment_text.size(), highlighted_text);

	//cursor should be at this position after update highlighted text
	int restore_pos = curr_segment_text_start_idx + highlighted_text.size();

	cursor_pos_change_ignore_flag = true;	//prevent setText() emitted cursor position change signal from fucking with completer
	setText(curr_text);

	cursor_pos_change_ignore_flag = true;	//prevent setCursorPosition() emitted cursor position change signal from fucking with completer
	//explicit set position to the end of highlighted text because setText() sets the cursor to the end of line edit
	setCursorPosition(restore_pos);
}

void 
MultiCompleterLineEdit::OnCursorPositionChanged(int old_pos, int new_pos) {

	//do not complete when user is changing cursor by mouse
	if (QApplication::mouseButtons() != Qt::MouseButton::NoButton) {
		return;
	}

	//do not complete if text is empty
	if (text().size() == 0) {
		return;
	}

	UpdateCurrentSegment();

	if (cursor_pos_change_ignore_flag) {
		cursor_pos_change_ignore_flag = false;
		return;
	}

	if (curr_segment_text.size() == 0) {
		QAbstractItemView *popup_view = completer->popup();
		if (popup_view->isVisible()) {
			popup_view->hide();
		}
	}
	else {
		completer->setCompletionPrefix(curr_segment_text);
		completer->complete();
	}
	
	//qDebug() << '(' << segment << ')';
}

//private:

void 
MultiCompleterLineEdit::GetCurrentSegmentText(QString* segment) {
	
	QString full_text = text();

	if (curr_segment_len <= 0) {
		*segment = "";
		return;
	}

	*segment = full_text.mid(curr_segment_start_idx, curr_segment_len);
}

void
MultiCompleterLineEdit::UpdateCurrentSegment() {

	//cursor position int is the index number of the character when entered at this cursor position

	QString full_text = text();

	int cursor_idx = cursorPosition();

	curr_segment_start_idx = cursor_idx;
	int curr_segment_end_idx = cursor_idx;
	curr_segment_len = 0;


	while (curr_segment_start_idx != 0) {

		curr_segment_start_idx--;

		if (separator_set.contains(full_text[curr_segment_start_idx])) {
			break;
		}
	}

	while (curr_segment_end_idx != full_text.size()) {
		if (separator_set.contains(full_text[curr_segment_end_idx])) {
			break;
		}

		curr_segment_end_idx++;
	}

	if (curr_segment_start_idx != 0 || separator_set.contains(full_text[curr_segment_start_idx])) {
		curr_segment_start_idx++;
	}

	curr_segment_len = curr_segment_end_idx - curr_segment_start_idx;

	if (curr_segment_len <= 0) {
		curr_segment_text = "";
		curr_segment_text_start_idx = -1;
		return;
	}

	curr_segment_text = full_text.mid(curr_segment_start_idx, curr_segment_len).trimmed();
	curr_segment_text_start_idx = -1;
	
	if (curr_segment_text.size() > 0) {
		curr_segment_text_start_idx = curr_segment_start_idx;
		while (full_text[curr_segment_text_start_idx] != curr_segment_text[0]) {
			curr_segment_text_start_idx++;
		}
	}
}