#include <QtTest>
#include <QSignalSpy>
#include <QDebug>

#include "../../filename_map.h"

class FilenameMapTest : public QObject
{
    Q_OBJECT

public:
    FilenameMapTest();
    ~FilenameMapTest();

private slots:

    void OnePatternOneTagBlock();
    void GetMappedTags();
    void OnePatternMultiTagBlock();
    void MultiPatternMultiTagBlock();

    void EmptyFile();
    void CommentOutsideBlock();
    void CommentInsideBlock();

    //incorrect syntax
    void BeginBeginBlock();     //Begin -> Begin -> X
    void BeginEndBlock();       //Begin -> End -> X
    void BeginTagBeginBlock();  //Begin -> Tag -> Begin keyword
    void TagBlock();            //Tag keyword as first keyword
    void EndBlock();            //End keyword as first keyword
    void NoPatternBlock();      //No patterns after begin keyword
    void NoTagBlock();          //No tags after tag keyword

    void SeekNextBegin();

private:

    Logger logger;
    void CreateMapFile(const QString& content);
};

FilenameMapTest::FilenameMapTest()
{
}

FilenameMapTest::~FilenameMapTest()
{
}

void
FilenameMapTest::OnePatternOneTagBlock() {
    CreateMapFile("BEGIN\npattern\nTAG\ntag\nEND\n");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QCOMPARE(map.GetSize(), 1);
}

void
FilenameMapTest::GetMappedTags() {
    CreateMapFile("BEGIN\n\\.gif$\nTAG\ntest\nEND\n");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QList<QString> tags;
    map.GetMappedTags("test.gif", &tags);

    QCOMPARE(tags.size(), 1);
    QCOMPARE(tags[0], "test");
}

void
FilenameMapTest::OnePatternMultiTagBlock() {
    CreateMapFile("BEGIN\n\\.gif$\n\\.webm$\nTAG\ntest\nEND\n");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QList<QString> tags;
    map.GetMappedTags("test.gif", &tags);

    QCOMPARE(tags.size(), 1);
    QCOMPARE(tags[0], "test");

    tags.clear();
    map.GetMappedTags("test.webm", &tags);
    QCOMPARE(tags.size(), 1);
    QCOMPARE(tags[0], "test");
}

void
FilenameMapTest::MultiPatternMultiTagBlock() {
    CreateMapFile("BEGIN\n\\.gif$\nTAG\ntest\ntest2\nEND\n");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QList<QString> tags;
    map.GetMappedTags("test.gif", &tags);

    QCOMPARE(tags.size(), 2);
    QCOMPARE(tags.contains("test"), true);
    QCOMPARE(tags.contains("test2"), true);
}

void
FilenameMapTest::EmptyFile() {
    CreateMapFile("");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QCOMPARE(map.GetSize(), 0);
}

void
FilenameMapTest::CommentOutsideBlock() {
    CreateMapFile("#test\nBEGIN\n\\.gif$\nTAG\ntest\nEND\n");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QList<QString> tags;
    map.GetMappedTags("test.gif", &tags);

    QCOMPARE(tags.size(), 1);
    QCOMPARE(tags[0], "test");
}

void
FilenameMapTest::CommentInsideBlock() {
    CreateMapFile("BEGIN\n\\.gif$\n#ssss\n#sssss\nTAG\ntest\nEND\n");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QList<QString> tags;
    map.GetMappedTags("test.gif", &tags);

    QCOMPARE(tags.size(), 1);
    QCOMPARE(tags[0], "test");
}


//incorrect syntax
void
FilenameMapTest::BeginBeginBlock() {
    CreateMapFile("BEGIN\npattern\nBEGIN\n");
    FilenameToTagMap map(&logger);

    QSignalSpy spy(&logger, &Logger::NewLogItemInserted);

    map.LoadFile();

    QCOMPARE(qvariant_cast<LogEntry>(spy.takeFirst().at(0)).content.contains("Map block is invalid"), true);
}

void
FilenameMapTest::BeginEndBlock() {
    CreateMapFile("BEGIN\npattern\nEND\n");
    FilenameToTagMap map(&logger);

    QSignalSpy spy(&logger, &Logger::NewLogItemInserted);

    map.LoadFile();

    QCOMPARE(qvariant_cast<LogEntry>(spy.takeFirst().at(0)).content.contains("Map block is invalid"), true);
}

void
FilenameMapTest::BeginTagBeginBlock() {
    CreateMapFile("BEGIN\npattern\nTAG\ntag\nBEGIN\n");
    FilenameToTagMap map(&logger);

    QSignalSpy spy(&logger, &Logger::NewLogItemInserted);

    map.LoadFile();

    QCOMPARE(qvariant_cast<LogEntry>(spy.takeFirst().at(0)).content.contains("Map block is invalid"), true);
}

void
FilenameMapTest::TagBlock() {
    CreateMapFile("TAG\ntag\n");
    FilenameToTagMap map(&logger);

    QSignalSpy spy(&logger, &Logger::NewLogItemInserted);

    map.LoadFile();

    QCOMPARE(qvariant_cast<LogEntry>(spy.takeFirst().at(0)).content.contains("Map block is invalid"), true);
}

void
FilenameMapTest::EndBlock() {
    CreateMapFile("END\n");
    FilenameToTagMap map(&logger);

    QSignalSpy spy(&logger, &Logger::NewLogItemInserted);

    map.LoadFile();

    QCOMPARE(qvariant_cast<LogEntry>(spy.takeFirst().at(0)).content.contains("Map block is invalid"), true);
}

void
FilenameMapTest::NoPatternBlock() {
    CreateMapFile("BEGIN\nTAG\ntag\nEND\n");
    FilenameToTagMap map(&logger);

    QSignalSpy spy(&logger, &Logger::NewLogItemInserted);

    map.LoadFile();

    QCOMPARE(qvariant_cast<LogEntry>(spy.takeFirst().at(0)).content.contains("Map block is invalid"), true);
}

void
FilenameMapTest::NoTagBlock() {
    CreateMapFile("BEGIN\npattern\nTAG\nEND\n");
    FilenameToTagMap map(&logger);

    QSignalSpy spy(&logger, &Logger::NewLogItemInserted);

    map.LoadFile();

    QCOMPARE(qvariant_cast<LogEntry>(spy.takeFirst().at(0)).content.contains("Map block is invalid"), true);
}

void
FilenameMapTest::SeekNextBegin() {
    CreateMapFile("END\nBEGIN\n\\.gif$\nTAG\ntest\nEND\n");
    FilenameToTagMap map(&logger);
    map.LoadFile();

    QList<QString> tags;
    map.GetMappedTags("test.gif", &tags);

    QCOMPARE(tags.size(), 1);
    QCOMPARE(tags[0], "test");
}

void
FilenameMapTest::CreateMapFile(const QString& content) {
    QFile f(FILENAME_MAP_FILE_NAME);
    QCOMPARE(f.open(QIODevice::WriteOnly), true);

    QTextStream stream(&f);
    stream << content;

    stream.flush();

    f.close();
}

QTEST_APPLESS_MAIN(FilenameMapTest)

#include "tst_filenamemaptest.moc"
