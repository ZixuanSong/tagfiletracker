#include <QtTest>
#include <QCompleter>
#include <QStringList>
#include <QSet>
#include <QDebug>

#include "../../multicompleterlineedit.h"

// add necessary includes here

class MultiCompleterLineEditTest : public QObject
{
    Q_OBJECT

public:
    MultiCompleterLineEditTest();
    ~MultiCompleterLineEditTest();

private slots:

    void init();
    void cleanup();

    void MatchAll();
    void MatchPart();

    void MatchStartToSep();
    void MatchBetweenSep();
    void MatchSeptoEnd();

    void HighlightReplaceSegment();

private:
    QStringList list;
    QCompleter* completer;
    MultiCompleterLineEdit* line_edit;

};

MultiCompleterLineEditTest::MultiCompleterLineEditTest()
{
    list << "Test1" << "Test2" << "Tesst1" << "Tesst2";
}

MultiCompleterLineEditTest::~MultiCompleterLineEditTest()
{

}


void
MultiCompleterLineEditTest::init() {
    //get fresh line edit and completer for each test
    completer = new QCompleter(list);
    completer->setFilterMode(Qt::MatchContains);
    completer->setMaxVisibleItems(10);

    line_edit = new MultiCompleterLineEdit();
    line_edit->SetCompleter(completer);
}

void
MultiCompleterLineEditTest::cleanup() {

    delete completer;
    delete line_edit;

    completer = nullptr;
    line_edit = nullptr;

}

void MultiCompleterLineEditTest::MatchAll()
{
        QTest::keyClick(line_edit, Qt::Key_T);

        QVERIFY(completer->completionCount() == list.size());

        QAbstractItemModel * completer_model = completer->completionModel();
        QSet<QString> completer_match_set;
        QModelIndex model_index;
        for(int i = 0; i < completer_model->rowCount(); i++){
            model_index = completer_model->index(i, 0);
            completer_match_set.insert(completer_model->data(model_index).toString());
        }

        QVERIFY(completer_match_set.contains("Test1") == true);
        QVERIFY(completer_match_set.contains("Test2") == true);
        QVERIFY(completer_match_set.contains("Tesst1") == true);
        QVERIFY(completer_match_set.contains("Tesst2") == true);

}

void MultiCompleterLineEditTest::MatchPart()
{
    QTest::keyClicks(line_edit, "Tess");

    QVERIFY(completer->completionCount() == 2);

    QAbstractItemModel * completer_model = completer->completionModel();
    QSet<QString> completer_match_set;
    QModelIndex model_index;
    for(int i = 0; i < completer_model->rowCount(); i++){
        model_index = completer_model->index(i, 0);
        completer_match_set.insert(completer_model->data(model_index).toString());
    }

    QVERIFY(completer_match_set.contains("Tesst1") == true);
    QVERIFY(completer_match_set.contains("Tesst2") == true);
}

void MultiCompleterLineEditTest::MatchStartToSep() {

    line_edit->setText("T,,");
    line_edit->AddSeparator(',');
    line_edit->setCursorPosition(0);

    QVERIFY(completer->completionCount() == list.size());
}

void MultiCompleterLineEditTest::MatchBetweenSep() {
    line_edit->setText(",T,");
    line_edit->AddSeparator(',');
    line_edit->setCursorPosition(2);

    QVERIFY(completer->completionCount() == list.size());
}

void MultiCompleterLineEditTest::MatchSeptoEnd() {
    line_edit->AddSeparator(',');
    line_edit->setText(",,T");

    QCOMPARE(completer->completionCount(), list.size());
}

void
MultiCompleterLineEditTest::HighlightReplaceSegment() {
    line_edit->setText("  Test");

    QTest::keyClick(QApplication::activePopupWidget(), Qt::Key_Down);

    QCOMPARE(line_edit->text(), "  Test1");
}

QTEST_MAIN(MultiCompleterLineEditTest)

#include "tst_multicompleterlineedittest.moc"
