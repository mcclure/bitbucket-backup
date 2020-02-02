#-------------------------------------------------
#
# SPROXEL sprite-ish voxel editor
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = sproxel
TEMPLATE = app

INCLUDEPATH += ../python/include
isEmpty(PYLIBS) {
LIBS += -L../python/libs -lpython27
}
!isEmpty(PYLIBS) { 
LIBS += $$PYLIBS
}

unix:!macx {
  CONFIG += link_pkgconfig
  PKGCONFIG += IlmBase
}

macx {
  INCLUDEPATH += /usr/local/include/OpenEXR
  LIBS += -lImath -lIex
}

win32 {
  INCLUDEPATH += ../IlmBase/include
  CONFIG(release) {
    LIBS += -L../IlmBase/lib/Release
  } else {
    LIBS += -L../IlmBase/lib/Debug
  }
  LIBS += -lImath -lIex
  DEFINES += NOMINMAX
  QMAKE_CXXFLAGS += -wd4996
}

SOURCES += \
    GLCamera.cpp \
    GLModelWidget.cpp \
    MainWindow.cpp \
    main.cpp \
    NewGridDialog.cpp \
    PreferencesDialog.cpp \
    PaletteWidget.cpp \
    LayersWidget.cpp \
    ProjectWidget.cpp \
    Tools.cpp \
    UndoManager.cpp \
    ImportExport.cpp \
    SproxelProject.cpp \
    script.cpp \
    pyConsole.cpp \
    pyBindings.cpp \
    pyImportExport.cpp

HEADERS  += \
    Foundation.h \
    GLCamera.h \
    GLModelWidget.h \
    GameVoxelGrid.h \
    VoxelGridGroup.h \
    SproxelProject.h \
    MainWindow.h \
    NewGridDialog.h \
    PreferencesDialog.h \
    PaletteWidget.h \
    LayersWidget.h \
    ProjectWidget.h \
    Tools.h \
    UndoManager.h \
    ImportExport.h \
    script.h \
    pyConsole.h \
    pyBindings.h \
    ConsoleWidget.h \
    glue/classGlue.h

FORMS += \
    NewGridDialog.ui

RESOURCES += \
    sproxel.qrc
