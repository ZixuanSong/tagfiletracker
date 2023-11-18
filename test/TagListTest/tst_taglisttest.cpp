#include <QtTest>
#include <QVector>
#include "../../tag_list.h"

// add necessary includes here

class TagListTest : public QObject
{
    Q_OBJECT

public:
    TagListTest();
    ~TagListTest();

private slots:
    void InsertSavedTagGetAllTags();
    void InsertNewTag();

    void RemoveTagById();
    void RemoveTagByName();

    void IsEmpty();
    void GetSize();

    void GetTagById();
    void GetTagByName();
    void GetTagIdByName();

    void TagExistByName();
    void TagExistById();

    void InsertWithOneHole();
    void InsertWithMultiHole();

    void InsertTagMedia();
    void RemoveTagMedia();

    void UpdateTagName();

};

TagListTest::TagListTest()
{

}

TagListTest::~TagListTest()
{

}

void
TagListTest::InsertSavedTagGetAllTags() {
    TagList list;

    Tag tag;
    tag.id = 0;
    tag.name = "test";

    list.InsertSavedTag(tag);

    QVector<Tag> res_list;
    list.GetAllTags(&res_list);

    QVERIFY(res_list.size() == 1);
    QVERIFY(res_list[0].id == 0);
    QVERIFY(res_list[0].name == "test");

}

void
TagListTest::InsertNewTag() {
    TagList list;

    unsigned int id;

    list.InsertNewTag("test", &id);

    QVector<Tag> res_list;
    list.GetAllTags(&res_list);

    QVERIFY(res_list.size() == 1);
    QVERIFY(res_list[0].id == id);
    QVERIFY(res_list[0].name == "test");
}

void
TagListTest::RemoveTagById() {

    TagList list;

    unsigned int id;
    list.InsertNewTag("test", &id);
    list.InsertNewTag("remove", &id);

    list.RemoveTagById(id);

    QVector<Tag> res_list;
    list.GetAllTags(&res_list);

    QVERIFY(res_list.size() == 1);
    QVERIFY(res_list[0].name == "test");

}

void
TagListTest::RemoveTagByName() {
    TagList list;

    unsigned int id;
    list.InsertNewTag("test", &id);
    list.InsertNewTag("remove", &id);

    list.RemoveTagByName("remove");

    QVector<Tag> res_list;
    list.GetAllTags(&res_list);

    QVERIFY(res_list.size() == 1);
    QVERIFY(res_list[0].name == "test");
}

void
TagListTest::IsEmpty() {
    TagList list;

    QVERIFY(list.Empty() == true);

    unsigned int id;
    list.InsertNewTag("test", &id);
    QVERIFY(list.Empty() == false);
}

void
TagListTest::GetSize() {
    TagList list;

    QVERIFY(list.GetSize() == 0);

    unsigned int id;
    list.InsertNewTag("test", &id);

    QVERIFY(list.GetSize() == 1);
}

void
TagListTest::GetTagById() {

    Tag tag;
    Tag res_tag;

    tag.id = 1;
    tag.name = "tag";
    tag.media_id_list.insert(4);

    TagList list;
    list.InsertSavedTag(tag);

    list.GetTagById(1, &res_tag);

    QVERIFY(res_tag.id = 1);
    QVERIFY(res_tag.name == "tag");
    QVERIFY(res_tag.media_id_list.size() == 1);
    QVERIFY(res_tag.media_id_list.values()[0] == 4);
}

void
TagListTest::GetTagByName() {
    Tag tag;
    Tag res_tag;

    tag.id = 1;
    tag.name = "tag";
    tag.media_id_list.insert(4);

    TagList list;
    list.InsertSavedTag(tag);

    list.GetTagByName("tag", &res_tag);

    QVERIFY(res_tag.id = 1);
    QVERIFY(res_tag.name == "tag");
    QVERIFY(res_tag.media_id_list.size() == 1);
    QVERIFY(res_tag.media_id_list.values()[0] == 4);
}

void
TagListTest::GetTagIdByName() {
    TagList list;

    unsigned int id;
    list.InsertNewTag("test", &id);

    unsigned int res;
    list.GetTagIdByName("test", &res);
    QVERIFY(id == res);
}

void
TagListTest::TagExistByName() {
    TagList list;
    unsigned int id;
    list.InsertNewTag("test", &id);

    QVERIFY(list.TagExistByName("test") == true);
    QVERIFY(list.TagExistByName("none") == false);
}

void
TagListTest::TagExistById() {
    TagList list;
    unsigned int id;
    list.InsertNewTag("test", &id);

    QVERIFY(list.TagExistById(id) == true);
    QVERIFY(list.TagExistById(100) == false);
}

void
TagListTest::InsertWithOneHole() {
    TagList list;

    Tag tag;
    tag.id = 0;
    tag.name = "tag0";

    list.InsertSavedTag(tag);

    tag.id = 2;
    tag.name = "tag2";

    list.InsertSavedTag(tag);

    QVERIFY(list.TagExistById(1) == false);

    unsigned int id;
    list.InsertNewTag("tag1", &id);
    QVERIFY(id == 1);

    list.InsertNewTag("tag3", &id);
    QVERIFY(id == 3);
}

void
TagListTest::InsertWithMultiHole(){
    TagList list;

    Tag tag;
    tag.id = 0;
    tag.name = "tag0";

    list.InsertSavedTag(tag);

    tag.id = 3;
    tag.name = "tag3";

    list.InsertSavedTag(tag);

    QVERIFY(list.TagExistById(1) == false);
    QVERIFY(list.TagExistById(2) == false);

    unsigned int id;
    list.InsertNewTag("tag1", &id);
    QVERIFY(id == 1);

    list.InsertNewTag("tag2", &id);
    QVERIFY(id == 2);
}

void
TagListTest::InsertTagMedia() {
    TagList list;

    unsigned int id;
    list.InsertNewTag("test", &id);

    list.InsertTagMedia(id, 0);

    Tag tag;
    list.GetTagById(id, &tag);

    QVERIFY(tag.media_id_list.values()[0] == 0);
}

void
TagListTest::RemoveTagMedia() {
    TagList list;

    unsigned int id;
    list.InsertNewTag("test", &id);

    list.InsertTagMedia(id, 0);
    list.RemoveTagMedia(0, 0);

    Tag tag;
    list.GetTagById(id, &tag);

    QVERIFY(tag.media_id_list.size() == 0);
}

void
TagListTest::UpdateTagName() {
    TagList list;

    unsigned int id;
    list.InsertNewTag("test", &id);
    list.UpdateTagName(id, "new");

    Tag tag;
    list.GetTagById(id, &tag);

    QVERIFY(tag.name == "new");
}

QTEST_APPLESS_MAIN(TagListTest)

#include "tst_taglisttest.moc"
