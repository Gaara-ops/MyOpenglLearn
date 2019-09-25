#-------------------------------------------------
#
# Project created by QtCreator 2019-09-12T16:29:21
#
#-------------------------------------------------

QT       += core gui
CONFIG += c++11
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LearnOpenGL
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    EventProcess.h

FORMS    += mainwindow.ui



FreeGlutDir=F:/opengl/freeglut-3.0.0/qt5.3/lib-release
#INCLUDEPATH += $${FreeGlutDir}/include

GlewDir = F:/opengl/glew-2.1.0/qt5.3/lib-release
#INCLUDEPATH += $${GlewDir}/include

GLMDir = F:/opengl/glm-0.9.9.4
#INCLUDEPATH += $${GLMDir}/

GLADDir = F:/opengl/glad/qt5.3/lib-release
#INCLUDEPATH += $${GLADDir}/include

STDDir = F:/vulkan/stb
#INCLUDEPATH += $${STDDir}/

GLFWDir = F:/opengl/glfw-3.2.1/qt5.3/lib-release
#INCLUDEPATH += $${GLFWDir}/include

SOILDir = F:/opengl/Simple_OpenGL_Image_Library/qt5.3
#INCLUDEPATH += $${SOILDir}/include

LearnOpenglDir = F:/opengl/learn/LearnOpenGL/includes
INCLUDEPATH += $${LearnOpenglDir}/


LIBS += -L$${FreeGlutDir}/lib libfreeglut
LIBS += -L$${GlewDir}/lib libglew32
LIBS += -L$${GLFWDir}/lib libglfw3
LIBS += -L$${GLADDir}/lib libglad
LIBS += -L$${SOILDir}/lib libSOIL
LIBS += -lopengl32 -lwinmm -lgdi32 -lm -lglu32
