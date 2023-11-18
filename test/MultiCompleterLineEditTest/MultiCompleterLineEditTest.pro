QT += testlib
QT += widgets
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_multicompleterlineedittest.cpp \
    ../../multicompleterlineedit.cpp \
    ../../GeneratedFiles/Debug/moc_multicompleterlineedit.cpp
