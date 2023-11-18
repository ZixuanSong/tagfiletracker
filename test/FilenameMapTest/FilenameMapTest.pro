QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_filenamemaptest.cpp \
    ../../filename_map.cpp \
    ../../logger.cpp \
    ../../GeneratedFiles/Debug/moc_logger.cpp
