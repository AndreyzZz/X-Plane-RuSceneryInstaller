#-------------------------------------------------
#
# Project created by QtCreator 2011-08-18T09:38:41
#
#-------------------------------------------------

CODECFORTR      = UTF-8
CODECFORSRC     = UTF-8

QT       += core gui network

TARGET = RuSceneryInstaller
TEMPLATE = app
TRANSLATIONS = RuSceneryInstaller_ru.ts

SOURCES += main.cpp\
        widget.cpp
HEADERS  += widget.h
FORMS    += widget.ui

win32:RC_FILE = RuSceneryInstaller.rc
macx:ICON = RuSceneryInstaller.icns

RESOURCES += RuSceneryInstaller.qrc

OTHER_FILES += \
    RuSceneryInstaller.rc \
    RuSceneryInstaller.ico
