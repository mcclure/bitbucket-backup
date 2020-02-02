# SPROXEL glue to PySide Python module

TARGET = SproxelGlue
TEMPLATE = lib
CONFIG += shared

win32 {
  TARGET_EXT = .pyd
  QMAKE_CXXFLAGS += -wd4522 -wd4800 -wd4100 -wd4244
}


INCLUDEPATH += ../../python/include
LIBS += -L../../python/libs -lpython27

INCLUDEPATH += ../../pyside/include/shiboken \
  ../../pyside/include/PySide \
  ../../pyside/include/PySide/QtCore \
  ../../pyside/include/PySide/QtGui

LIBS += -L../../pyside/lib -lshiboken-python2.7 -lpyside-python2.7

SOURCES += glue.cpp
HEADERS += classGlue.h
