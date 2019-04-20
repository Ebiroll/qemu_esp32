#DEFINES += "LINUX"
#DEFINES += "VSG_USE_NATIVE_STL"
#OSFLAG=linux
#CONFIG += release

#QMAKE_CXXFLAGS_RELEASE -= -O2
#QMAKE_CXXFLAGS_RELEASE += -Os

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += include

SOURCES +=  main.cpp \
            mainwindow.cpp

HEADERS +=document/commands/hexcommand.h \
    document/commands/insertcommand.h \
    document/commands/removecommand.h \
    document/commands/replacecommand.h \
    document/buffer/qhexbuffer.h \
    document/buffer/qmemoryrefbuffer.h \
    document/buffer/qmemorybuffer.h \
    document/qhexcursor.h \
    document/qhexdocument.h \
    document/qhexmetadata.h \
    document/qhexrenderer.h \
    qhexview.h \
    mainwindow.h


SOURCES +=  document/commands/hexcommand.cpp  \
    document/commands/insertcommand.cpp \
    document/commands/removecommand.cpp \
    document/commands/replacecommand.cpp \
    document/buffer/qhexbuffer.cpp \
    document/buffer/qmemoryrefbuffer.cpp \
    document/buffer/qmemorybuffer.cpp \
    document/qhexcursor.cpp \
    document/qhexdocument.cpp \
    document/qhexmetadata.cpp \
    document/qhexrenderer.cpp \
    qhexview.cpp
			

win32 {
#QMAKE_CXXFLAGS += -m32 -msse -mwindows
}

!win32 {
#LIBS += -static-libstdc++
}

TARGET = qdiff
CONFIG += qt
QT += network
#QT += opengl

QMAKE_CXXFLAGS += -fpermissive -std=c++11
#QMAKE_CXXFLAGS += -Wall

#DESTDIR = ./release
OBJECTS_DIR = ./build/.obj
MOC_DIR = ./build/.moc
RCC_DIR = ./build/.rcc
UI_DIR = ./build/.ui

