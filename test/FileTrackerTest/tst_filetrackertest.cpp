#include <QtTest>
#include <QString>
#include <QFile>
#include <QDir>
#include "../../file_tracker.h"

// add necessary includes here

class FileTrackerTest : public QObject
{
    Q_OBJECT

public:
    FileTrackerTest();
    ~FileTrackerTest();

private slots:
    void GetFileLongShortNameLongName();
    void GetFileLongShortNameShortName();
    void GetFileLongShortNameNonExist();

    void RootDirTrue();
    void RootDirFalse();
    void RootDirShortNameTrue();
    void RootDirShortNameFalse();
    void SubdirTrue();
    void SubdirFalse();
    void MultiSubdirTrue();
    void MultiSubdirFalse();

    void UpdateDirName();
    void UpdateDirNameWithShortName();
    void UpdateDirSubDir();
    void RemoveDir();

    void GetDirName();

    void AddMedia();
    void RemoveMedia();

    void GetDirMediaIdRecurNoChildDir();
    void GetDirMediaIdRecurOneLevelChildDir();
    void GetDirMediaIdRecurOneLevelMultiChildDir();
    void GetDirMediaIdRecurMultiLevelChildDir();
};

FileTrackerTest::FileTrackerTest()
{

}

FileTrackerTest::~FileTrackerTest()
{

}

void
FileTrackerTest::GetFileLongShortNameLongName() {

    QString name = "longfilenametogetshortname";
    QFile new_file(name);
    new_file.open(QIODevice::WriteOnly);
    new_file.close();

    QString long_name;
    QString short_name;

    QString abs_path = QDir::toNativeSeparators((QFileInfo(new_file).absoluteFilePath()));
    FileTracker::GetFileLongShortName(abs_path, &long_name, &short_name);

    QVERIFY(long_name == name);
    QVERIFY(short_name == "LONGFI~1");

    new_file.remove();
}

void
FileTrackerTest::GetFileLongShortNameShortName() {
    QString name = "file";
    QFile new_file(name);
    new_file.open(QIODevice::WriteOnly);
    new_file.close();

    QString long_name;
    QString short_name;

    QString abs_path = QDir::toNativeSeparators((QFileInfo(new_file).absoluteFilePath()));
    FileTracker::GetFileLongShortName(abs_path, &long_name, &short_name);

    QVERIFY(long_name == name);
    QVERIFY(short_name.size() == 0);

    new_file.remove();
}

void
FileTrackerTest::GetFileLongShortNameNonExist() {

    QString long_name;
    QString short_name;

    QString abs_path = QDir::currentPath();
    abs_path.replace('/', '\\');
    abs_path.append('\\');
    abs_path.append("non_exist_file");

    QVERIFY(FileTracker::GetFileLongShortName(abs_path, &long_name, &short_name) < 0);
}

void
FileTrackerTest::RootDirTrue() {
    FileTracker tracker;

    QDir curr_dir = QDir::current();
    curr_dir.mkdir("dir1");

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());

    tracker.SetRootDir(root_abs_path);
    tracker.AddDirAbsPath(root_abs_path, "dir1", "");

    QVERIFY(tracker.DirExist("dir1") == true);

    curr_dir.rmdir("dir1");
}

void
FileTrackerTest::RootDirFalse() {
    FileTracker tracker;

    QDir curr_dir = QDir::current();

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());

    tracker.SetRootDir(root_abs_path);

    QVERIFY(tracker.DirExist("dir1") == false);
}

void
FileTrackerTest::RootDirShortNameTrue() {
    FileTracker tracker;

    QDir curr_dir = QDir::current();
    curr_dir.mkdir("longnamedir");

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());

    tracker.SetRootDir(root_abs_path);

    tracker.AddDirAbsPath(root_abs_path, "longnamedir", "LONGNA~1");

    QVERIFY(tracker.DirExist("LONGNA~1") == true);

    curr_dir.rmdir("longnamedir");
}

void
FileTrackerTest::RootDirShortNameFalse() {
    FileTracker tracker;

    QDir curr_dir = QDir::current();

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());

    tracker.SetRootDir(root_abs_path);

    QVERIFY(tracker.DirExist("LONGNA~1") == false);

}

void
FileTrackerTest::SubdirTrue() {
    FileTracker tracker;

    QDir curr_dir = QDir::current();

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());
    tracker.SetRootDir(root_abs_path);

    curr_dir.mkdir("dir1");

    tracker.AddDirAbsPath(root_abs_path, "dir1", "");

    curr_dir.cd("dir1");

    curr_dir.mkdir("dir2");

    tracker.AddDirAbsPath(root_abs_path.append("\\dir1"), "dir2", "");

    QVERIFY(tracker.DirExist("dir1\\dir2") == true);

    curr_dir.removeRecursively();
}

void
FileTrackerTest::SubdirFalse(){
    FileTracker tracker;

    QDir curr_dir = QDir::current();

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());
    tracker.SetRootDir(root_abs_path);

    curr_dir.mkdir("dir1");

    tracker.AddDirAbsPath(root_abs_path, "dir1", "");

    curr_dir.cd("dir1");

    curr_dir.mkdir("dir2");

    tracker.AddDirAbsPath(root_abs_path.append("\\dir1"), "dir2", "");

    QVERIFY(tracker.DirExist("dir1\\dir3") == false);

    curr_dir.removeRecursively();
}

void
FileTrackerTest::MultiSubdirTrue() {
    FileTracker tracker;

    QDir curr_dir = QDir::current();

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());
    tracker.SetRootDir(root_abs_path);


    curr_dir.mkdir("dir1");

    tracker.AddDirAbsPath(root_abs_path, "dir1", "");

    curr_dir.cd("dir1");

    curr_dir.mkdir("dir2");

    tracker.AddDirAbsPath(root_abs_path.append("\\dir1"), "dir2", "");


    curr_dir.cd("dir2");
    curr_dir.mkdir("dir3");

    tracker.AddDirAbsPath(root_abs_path.append("\\dir2"), "dir3", "");


    QVERIFY(tracker.DirExist("dir1\\dir2\\dir3") == true);

    curr_dir.cdUp();
    curr_dir.removeRecursively();
}

void
FileTrackerTest::MultiSubdirFalse() {
    FileTracker tracker;

    QDir curr_dir = QDir::current();

    QString root_abs_path = QDir::toNativeSeparators(curr_dir.absolutePath());
    tracker.SetRootDir(root_abs_path);


    curr_dir.mkdir("dir1");

    tracker.AddDirAbsPath(root_abs_path, "dir1", "");

    curr_dir.cd("dir1");

    curr_dir.mkdir("dir2");

    tracker.AddDirAbsPath(root_abs_path.append("\\dir1"), "dir2", "");


    curr_dir.cd("dir2");
    curr_dir.mkdir("dir3");

    tracker.AddDirAbsPath(root_abs_path.append("\\dir2"), "dir3", "");


    QVERIFY(tracker.DirExist("dir1\\dir2\\dir4") == false);

    curr_dir.cdUp();
    curr_dir.removeRecursively();
}

void FileTrackerTest::UpdateDirName() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "dir1", "");
    tracker.UpdateDirName("dir1", "dir2", "");

    QVERIFY(tracker.DirExist("dir1") == false);
    QVERIFY(tracker.DirExist("dir2") == true);
}

void FileTrackerTest::UpdateDirNameWithShortName() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "dir1", "S1");
    tracker.UpdateDirName("dir1", "dir2", "S2");

    QVERIFY(tracker.DirExist("S1") == false);
    QVERIFY(tracker.DirExist("S2") == true);

}

void FileTrackerTest::UpdateDirSubDir() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "dir1", "");
    tracker.AddDirSubPath("", "dir2", "");
    tracker.AddDirSubPath("dir1", "dir3", "");

    tracker.UpdateDirSubdir("dir1\\dir3", "dir2");

    QVERIFY(tracker.DirExist("dir1\\dir3") == false);
    QVERIFY(tracker.DirExist("dir2\\dir3") == true);
}

void FileTrackerTest::RemoveDir() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "dir1", "");
    tracker.AddDirSubPath("dir1", "dir3", "");

    tracker.RemoveDir("dir1");

    QVERIFY(tracker.DirExist("dir1\\dir3") == false);
    QVERIFY(tracker.DirExist("dir1") == false);

}

void FileTrackerTest::GetDirName() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "Dirname", "Shortname");

    QString l_out, s_out;

    tracker.GetDirName("\\Dirname", &l_out, &s_out);

    QVERIFY(l_out == "Dirname");
    QVERIFY(s_out == "Shortname");

    tracker.GetDirName("\\Shortname", &l_out, &s_out);
    QVERIFY(l_out == "Dirname");
    QVERIFY(s_out == "Shortname");
}

void FileTrackerTest::AddMedia() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddMediaSubPath("", 1);

    QList<unsigned int> media_list;
    tracker.GetDirMediaIdRecurByPath("", &media_list);

    QVERIFY(media_list.size() == 1);
    QVERIFY(media_list.contains(1));


}

void FileTrackerTest::RemoveMedia() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddMediaSubPath("", 1);
    tracker.RemoveMedia("", 1);

    QList<unsigned int> media_list;
    tracker.GetDirMediaIdRecurByPath("", &media_list);

    QVERIFY(media_list.size() == 0);
}


void FileTrackerTest::GetDirMediaIdRecurNoChildDir() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddMediaSubPath("", 1);
    tracker.AddMediaSubPath("", 2);
    tracker.AddMediaSubPath("", 3);
    tracker.AddMediaSubPath("", 4);

    QList<unsigned int> media_list;
    tracker.GetDirMediaIdRecurByPath("", &media_list);

    QVERIFY(media_list.size() == 4);
    QVERIFY(media_list.contains(1));
    QVERIFY(media_list.contains(2));
    QVERIFY(media_list.contains(3));
    QVERIFY(media_list.contains(4));
}

void FileTrackerTest::GetDirMediaIdRecurOneLevelChildDir() {

    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "dir1", "");

    tracker.AddMediaSubPath("", 1);
    tracker.AddMediaSubPath("", 2);
    tracker.AddMediaSubPath("dir1", 3);
    tracker.AddMediaSubPath("dir1", 4);

    QList<unsigned int> media_list;
    tracker.GetDirMediaIdRecurByPath("", &media_list);

    QVERIFY(media_list.size() == 4);
    QVERIFY(media_list.contains(1));
    QVERIFY(media_list.contains(2));
    QVERIFY(media_list.contains(3));
    QVERIFY(media_list.contains(4));

}

void FileTrackerTest::GetDirMediaIdRecurOneLevelMultiChildDir() {

    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "dir1", "");
    tracker.AddDirSubPath("", "dir2", "");

    tracker.AddMediaSubPath("", 1);
    tracker.AddMediaSubPath("", 2);
    tracker.AddMediaSubPath("dir1", 3);
    tracker.AddMediaSubPath("dir1", 4);
    tracker.AddMediaSubPath("dir2", 5);
    tracker.AddMediaSubPath("dir2", 6);

    QList<unsigned int> media_list;
    tracker.GetDirMediaIdRecurByPath("", &media_list);

    QVERIFY(media_list.size() == 6);
    QVERIFY(media_list.contains(1));
    QVERIFY(media_list.contains(2));
    QVERIFY(media_list.contains(3));
    QVERIFY(media_list.contains(4));
    QVERIFY(media_list.contains(5));
    QVERIFY(media_list.contains(6));
}

void FileTrackerTest::GetDirMediaIdRecurMultiLevelChildDir() {
    FileTracker tracker;

    QString root_abs_path = QDir::toNativeSeparators(QDir::currentPath());
    tracker.SetRootDir(root_abs_path);

    tracker.AddDirSubPath("", "dir1", "");
    tracker.AddDirSubPath("dir1", "dir2", "");

    tracker.AddMediaSubPath("", 1);
    tracker.AddMediaSubPath("", 2);
    tracker.AddMediaSubPath("dir1", 3);
    tracker.AddMediaSubPath("dir1", 4);
    tracker.AddMediaSubPath("dir1\\dir2", 5);
    tracker.AddMediaSubPath("dir1\\dir2", 6);

    QList<unsigned int> media_list;
    tracker.GetDirMediaIdRecurByPath("", &media_list);

    QVERIFY(media_list.size() == 6);
    QVERIFY(media_list.contains(1));
    QVERIFY(media_list.contains(2));
    QVERIFY(media_list.contains(3));
    QVERIFY(media_list.contains(4));
    QVERIFY(media_list.contains(5));
    QVERIFY(media_list.contains(6));

}

QTEST_APPLESS_MAIN(FileTrackerTest)

#include "tst_filetrackertest.moc"
