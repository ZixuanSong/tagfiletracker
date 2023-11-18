#include <QtTest>
#include <QString>
#include "../../track_ignore.h"

// add necessary includes here

class IgnoreListTest : public QObject
{
    Q_OBJECT

public:
    IgnoreListTest();
    ~IgnoreListTest();

    IgnoreList list;

private slots:

    void IsConcreteTrue();
    void IsConcreteBeginFalse();
    void IsConcreteMiddleFalse();
    void IsConcreteEndFalse();

    void MatchWildcardMatchAllTrue();
    void MatchWildcardBeginTrue();
    void MatchWildcardBeginFalse();
    void MatchWildcardEndTrue();
    void MatchWildcardEndFalse();
    void MatchWildcardMiddleTrue();
    void MatchWildcardMiddleFalse();
    void MatchWildcardMultiMiddleTrue();
    void MatchWildcardMultiMiddleFalse();
    void MatchWildcardBeginMiddleTrue();
    void MatchWildcardBeginMiddleFalse();
    void MatchWildcardMiddleEndTrue();
    void MatchWildcardMiddleEndFalse();
    void MatchWildcardBeginMiddleEndTrue();
    void MatchWildcardBeginMiddleEndFalse();
    void MatchWildcardBeginMultiMiddleEndTrue();
    void MatchWildcardBeginMultiMiddleEndFalse();

    void MatchIgnoreRootOnly();
    void MatchIgnoreSubdir();
    void MatchIgnoreDeepSubdir();
    void MatchIgnoreRegression();

};

IgnoreListTest::IgnoreListTest()
{

}

IgnoreListTest::~IgnoreListTest()
{

}


//IsConcrete test cases
void
IgnoreListTest::IsConcreteTrue() {
    QVERIFY(IgnoreList::IsConcrete("concrete") == true);
}

void
IgnoreListTest::IsConcreteBeginFalse() {
    QVERIFY(IgnoreList::IsConcrete("*ildcard") == false);
}

void
IgnoreListTest::IsConcreteMiddleFalse() {
    QVERIFY(IgnoreList::IsConcrete("wil*card") == false);
}

void
IgnoreListTest::IsConcreteEndFalse() {
    QVERIFY(IgnoreList::IsConcrete("wildcar*") == false);
}

//MatchWildcard test cases
void
IgnoreListTest::MatchWildcardMatchAllTrue()
{
    QVERIFY(IgnoreList::MatchWildcard("*", "anything") == true);
}

void
IgnoreListTest::MatchWildcardBeginTrue() {
    QVERIFY(IgnoreList::MatchWildcard("*est", "test") == true);
}

void
IgnoreListTest::MatchWildcardBeginFalse() {
    QVERIFY(IgnoreList::MatchWildcard("*est", "teat") == false);
}
void
IgnoreListTest::MatchWildcardEndTrue() {
    QVERIFY(IgnoreList::MatchWildcard("tes*", "test") == true);
}

void
IgnoreListTest::MatchWildcardEndFalse() {
    QVERIFY(IgnoreList::MatchWildcard("tes*", "teas") == false);
}
void
IgnoreListTest::MatchWildcardMiddleTrue() {
    QVERIFY(IgnoreList::MatchWildcard("te*t", "test") == true);
}

void
IgnoreListTest::MatchWildcardMiddleFalse(){
    QVERIFY(IgnoreList::MatchWildcard("te*t", "tesa") == false);
}

void
IgnoreListTest::MatchWildcardMultiMiddleTrue(){
    QVERIFY(IgnoreList::MatchWildcard("m*ret*st", "moretest") == true);
}

void
IgnoreListTest::MatchWildcardMultiMiddleFalse(){
    QVERIFY(IgnoreList::MatchWildcard("m*ret*st", "most") == false);
}

void
IgnoreListTest::MatchWildcardBeginMiddleTrue(){
    QVERIFY(IgnoreList::MatchWildcard("*e*t", "test") == true);
}

void
IgnoreListTest::MatchWildcardBeginMiddleFalse(){
    QVERIFY(IgnoreList::MatchWildcard("*e*t", "ttttessssa") == false);
}

void
IgnoreListTest::MatchWildcardMiddleEndTrue(){
    QVERIFY(IgnoreList::MatchWildcard("t*s*", "test") == true);
}

void
IgnoreListTest::MatchWildcardMiddleEndFalse(){
    QVERIFY(IgnoreList::MatchWildcard("t*s*", "taaaabt") == false);
}

void
IgnoreListTest::MatchWildcardBeginMiddleEndTrue(){
    QVERIFY(IgnoreList::MatchWildcard("*e*man*", "germany") == true);
}

void
IgnoreListTest::MatchWildcardBeginMiddleEndFalse(){
    QVERIFY(IgnoreList::MatchWildcard("*e*man*", "germclean") == false);
}

void
IgnoreListTest::MatchWildcardBeginMultiMiddleEndTrue(){
    QVERIFY(IgnoreList::MatchWildcard("*o*ngt*s*", "longtest") == true);
}

void
IgnoreListTest::MatchWildcardBeginMultiMiddleEndFalse(){
    QVERIFY(IgnoreList::MatchWildcard("*o*ngt*s*", "lsngtest") == false);
}

//MatchIgnore tests
void
IgnoreListTest::MatchIgnoreRootOnly() {
    list.ParseIgnoreFileLine("concrete1");
    list.ParseIgnoreFileLine("concrete2");
    list.ParseIgnoreFileLine("*.type");
    list.ParseIgnoreFileLine("*.type2");
    list.ParseIgnoreFileLine("f*");


    QVERIFY(list.MatchIgnore("concrete1") == true);
    QVERIFY(list.MatchIgnore("sven.type") == true);
    QVERIFY(list.MatchIgnore("f1") == true);
}

void
IgnoreListTest::MatchIgnoreSubdir() {
    list.ParseIgnoreFileLine("dir1/");
    list.ParseIgnoreFileLine(("dir2/concrete1"));
    list.ParseIgnoreFileLine("dir3/*.type");

    QVERIFY(list.MatchIgnore("dir1\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir1\\dir2\\concrete2") == true);
    QVERIFY(list.MatchIgnore("dir2\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir3\\dll.type") == true);
}

void
IgnoreListTest::MatchIgnoreDeepSubdir() {
    list.ParseIgnoreFileLine("dir4/dir1/dir1/dir1/");
    list.ParseIgnoreFileLine("dir5/dir1/*png");

    QVERIFY(list.MatchIgnore("dir4\\dir1\\dir1\\dir1\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir4\\dir1\\dir1\\dir1\\dir1\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir5\\dir1\\spng") == true);
}

void
IgnoreListTest::MatchIgnoreRegression() {


    QVERIFY(list.MatchIgnore("concrete1") == true);
    QVERIFY(list.MatchIgnore("sven.type") == true);
    QVERIFY(list.MatchIgnore("f1") == true);
    QVERIFY(list.MatchIgnore("dir1\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir1\\dir2\\concrete2") == true);
    QVERIFY(list.MatchIgnore("dir2\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir3\\dll.type") == true);
    QVERIFY(list.MatchIgnore("dir4\\dir1\\dir1\\dir1\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir4\\dir1\\dir1\\dir1\\dir1\\concrete1") == true);
    QVERIFY(list.MatchIgnore("dir5\\dir1\\spng") == true);

    QVERIFY(list.MatchIgnore("concrete4") == false);
    QVERIFY(list.MatchIgnore("test.type4") == false);
    QVERIFY(list.MatchIgnore("sfile") == false);
    QVERIFY(list.MatchIgnore("dir2\\dir1\\test") == false);
    QVERIFY(list.MatchIgnore("dir2\\concrete2") == false);
    QVERIFY(list.MatchIgnore("dir3\\dir1\\test.type") == false);
    QVERIFY(list.MatchIgnore("dir3\\concrete") == false);
    QVERIFY(list.MatchIgnore("concrete4") == false);
    QVERIFY(list.MatchIgnore("dir4\\dir1\\test") == false);
    QVERIFY(list.MatchIgnore("dir4\\dir1\\dir1\\test") == false);
    QVERIFY(list.MatchIgnore("dir5\\test") == false);
    QVERIFY(list.MatchIgnore("dir5\\dir1\\test") == false);
}

QTEST_APPLESS_MAIN(IgnoreListTest)

#include "tst_ignorelisttest.moc"
