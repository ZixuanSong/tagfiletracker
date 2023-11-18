#include <QtTest>
#include "../../media_list.h"

class MediaListTest : public QObject
{
    Q_OBJECT

public:
    MediaListTest();
    ~MediaListTest();

private slots:
    void InsertMediaGetAllMediaPtr();
    void RemoveMedia();

    void GetSize();

    void MediaExistById();
    void MediaExistBySubpathName();

    void InsertMediaTagGetMediaTagIds();
    void RemoveMediaTag();
    void GetMediaTagCount();
    void MediaTagIdExist();

    void GetMediaById();
    void GetMediaBySubpathName();
    void GetMediaInfoById();
    void GetMediaInfoBySubpathName();

    void UpdateMediaName();
    void UpdateMediaSubdir();
    void UpdateMediaHash();

};

MediaListTest::MediaListTest()
{

}

MediaListTest::~MediaListTest()
{

}

void
MediaListTest::InsertMediaGetAllMediaPtr() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    QVector<Media*> res;
    list.GetAllMediaPtr(&res);

    QVERIFY(res[0]->info.id == 1);
}

void
MediaListTest::RemoveMedia() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    list.RemoveMedia(1);

    QVector<Media*> res;
    list.GetAllMediaPtr(&res);

    QVERIFY(res.size() == 0);
}

void
MediaListTest::GetSize() {
    MediaList list;

    QVERIFY(list.GetSize() == 0);

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    QVERIFY(list.GetSize() == 1);
}

void
MediaListTest::MediaExistById() {
    MediaList list;

    QVERIFY(list.MediaExistById(0) == false);

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    QVERIFY(list.MediaExistById(1) == true);
}

void
MediaListTest::MediaExistBySubpathName() {
    MediaList list;

    QVERIFY(list.MediaExistBySubpathName("none") == false);

    MediaInfo media;
    media.id = 1;
    media.sub_path = "\\subpath";
    media.long_name = "longname";

    list.InsertMedia(media);

    QVERIFY(list.MediaExistBySubpathName("\\subpath\\longname") == true);
}

void
MediaListTest::InsertMediaTagGetMediaTagIds() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    list.InsertMediaTag(0, 1);

    QVector<unsigned int> res;
    list.GetMediaTagIds(1, &res);

    QVERIFY(res[0] == 0);
}

void
MediaListTest::RemoveMediaTag() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    list.InsertMediaTag(0, 1);
    list.RemoveMediaTag(0, 1);

    QVector<unsigned int> res;
    list.GetMediaTagIds(1, &res);

    QVERIFY(res.size() == 0);
}

void
MediaListTest::GetMediaTagCount() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    list.InsertMediaTag(0, 1);

    QVector<unsigned int> res;
    list.GetMediaTagIds(1, &res);

    QVERIFY(list.GetMediaTagCount(1) == 1);
}

void
MediaListTest::MediaTagIdExist() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);

    list.InsertMediaTag(0, 1);

    QVERIFY(list.MediaTagIdExist(1, 0) == true);
    QVERIFY(list.MediaTagIdExist(1, 1) == false);
}

void
MediaListTest::GetMediaById() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);
    list.InsertMediaTag(1, 1);

    Media res;
    list.GetMediaById(1, &res);

    QVERIFY(res.info.id == 1);
    QVERIFY(res.tag_id_list.values()[0] == 1);
}

void
MediaListTest::GetMediaBySubpathName() {
    MediaList list;

    MediaInfo media;
    media.id = 1;
    media.sub_path = "\\subpath";
    media.long_name = "longname";

    list.InsertMedia(media);
    list.InsertMediaTag(1, 1);

    Media res;
    list.GetMediaBySubpathName("\\subpath\\longname", &res);

    QVERIFY(res.info.id == 1);
    QVERIFY(res.tag_id_list.values()[0] == 1);
}

void
MediaListTest::GetMediaInfoById() {
    MediaList list;

    MediaInfo media;
    media.id = 1;
    media.long_name = "name";
    media.short_name = "alt";
    media.hash = "hash";

    list.InsertMedia(media);

    MediaInfo res;
    list.GetMediaInfoById(1, &res);

    QVERIFY(res.id == 1);
    QVERIFY(res.long_name == "name");
    QVERIFY(res.short_name == "alt");
    QVERIFY(res.hash == "hash");
}

void
MediaListTest::GetMediaInfoBySubpathName() {
    MediaList list;

    MediaInfo media;
    media.id = 1;
    media.sub_path = "\\subpath";
    media.long_name = "name";
    media.short_name = "alt";
    media.hash = "hash";

    list.InsertMedia(media);

    MediaInfo res;
    list.GetMediaInfoBySubpathName("\\subpath\\name", &res);

    QVERIFY(res.id == 1);
    QVERIFY(res.long_name == "name");
    QVERIFY(res.short_name == "alt");
    QVERIFY(res.hash == "hash");
}

void
MediaListTest::UpdateMediaName() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);
    list.UpdateMediaName(1, "longname", "shortname");

    MediaInfo info;
    list.GetMediaInfoById(1, &info);

    QVERIFY(info.long_name == "longname");
    QVERIFY(info.short_name == "shortname");
}

void
MediaListTest::UpdateMediaSubdir() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);
    list.UpdateMediaSubdir(1, "\\subdir");

    MediaInfo info;
    list.GetMediaInfoById(1, &info);
    QVERIFY(info.sub_path == "\\subdir");
}

void
MediaListTest::UpdateMediaHash() {
    MediaList list;

    MediaInfo media;
    media.id = 1;

    list.InsertMedia(media);
    list.UpdateMediaHash(1, "hash");

    MediaInfo info;
    list.GetMediaInfoById(1, &info);

    QVERIFY(info.hash == "hash");
}

QTEST_APPLESS_MAIN(MediaListTest)

#include "tst_medialisttest.moc"
