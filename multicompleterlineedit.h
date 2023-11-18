#pragma once
#include <QLineEdit>
#include <QCompleter>
#include <QSet>
#include <QList>

class MultiCompleterLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	MultiCompleterLineEdit(QWidget *parent = nullptr);
	~MultiCompleterLineEdit();

	void SetCompleter(QCompleter* completer);
	void AddSeparator(const QChar separator);
	void AddSeparatorList(const QList<QChar>& sep_list);

private slots:

	void OnTextEdited(const QString& new_text);
	void OnCompleterHighlighted(const QString& highlighted_text);
	void OnCursorPositionChanged(int old_pos, int new_pos);

private:

	QCompleter*		completer;
	QSet<QChar>		separator_set;
	//QList<int>		separator_index_list;

	int	curr_segment_start_idx;
	int curr_segment_len;

	QString curr_segment_text;
	int curr_segment_text_start_idx;
	//int curr_segment_end_idx;

	bool	cursor_pos_change_ignore_flag;

	void GetCurrentSegmentText(QString* segment);
	void UpdateCurrentSegment();
};

