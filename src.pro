DEFINES += SYNERGYPLUGIN_LIBRARY

# SynergyPlugin files

SOURCES += \
    synergyplugin.cpp \
    synergycontrol.cpp \
    tasksdialog.cpp \
    asyncworker.cpp \
    taskwizard.cpp \
    settingspage.cpp \
    synergysettings.cpp \
    syncdialog.cpp \
    filestosyncdialog.cpp

HEADERS += \
    synergyplugin.h \
    synergyconstants.h \
    synergy_global.h \
    synergycontrol.h \
    tasksdialog.h \
    asyncworker.h \
    taskwizard.h \
    settingspage.h \
    synergysettings.h \
    syncdialog.h \
    filestosyncdialog.h

# Qt Creator linking

## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=C:/qt-creator-opensource-src-3.3.1

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=c:\build-qtcreator-Desktop_Qt_5_4_1_MSVC2013_32bit-Release

## uncomment to build plugin into user config directory
## %LOCALAPPDATA%/plugins/<ideversion>
##    where <localappdata> is e.g.
##   "%LOCALAPPDATA%\qtcreator" on Windows Vista and later
##    "$XDG_DATA_HOME/data/QtProject/qtcreator" or "~/.local/share/data/QtProject/qtcreator" on Linux
##    "~/Library/Application Support/QtProject/Qt Creator" on Mac
# USE_USER_DESTDIR = yes

###### If the plugin can be depended upon by other plugins, this code needs to be outsourced to
###### <dirname>_dependencies.pri, where <dirname> is the name of the directory containing the
###### plugin's sources.

QTC_PLUGIN_NAME = SynergyPlugin
QTC_LIB_DEPENDS += \
    extensionsystem \
    utils

QTC_PLUGIN_DEPENDS += \
    coreplugin \
    texteditor \
    coreplugin \
    vcsbase \
    diffeditor

QTC_PLUGIN_RECOMMENDS += \
    # optional plugin dependencies. nothing here at this time

###### End _dependencies.pri contents ######

include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)

FORMS += \
    tasksdialog.ui \
    taskwizard.ui \
    settingspage.ui \
    syncdialog.ui \
    filestosyncdialog.ui

RESOURCES += \
    synergy.qrc

DISTFILES += \
    SynergyPlugin.pluginspec.in
