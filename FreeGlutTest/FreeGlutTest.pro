#-------------------------------------------------
#
# Project created by QtCreator 2019-11-05T14:52:30
#
#-------------------------------------------------

QT       += core gui

TARGET = FreeGlutTest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp


FreeGlutDir=F:/opengl/freeglut-3.0.0/qt5.3/lib-release
INCLUDEPATH += $${FreeGlutDir}/include

GlewDir = F:/opengl/glew-2.1.0/qt5.3/lib-release
INCLUDEPATH += $${GlewDir}/include

GLMDir = F:/opengl/glm-0.9.9.4
INCLUDEPATH += $${GLMDir}/

LIBS += -L$${FreeGlutDir}/lib libfreeglut
LIBS += -L$${GlewDir}/lib libglew32

LIBS += -lopengl32 -lwinmm -lgdi32 -lm -lglu32
