#-------------------------------------------------
#
# Project created by QtCreator 2019-04-16T08:44:48
#
#-------------------------------------------------



QT       += core gui

CONFIG += c++11
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MyOpenglProject
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    include/display.cpp \
    include/init.cpp \
    include/shaders.cpp \
    CubicSplineSegment.cpp \
    Trackball.cpp \
    TransferFunctionControlPoint.cpp \
    imageloader.cpp

HEADERS  += mainwindow.h \
    include/display.hpp \
    include/init.hpp \
    include/scene.hpp \
    include/shaders.hpp \
    CubicSpline.h \
    CubicSplineSegment.h \
    fixedcamera.h \
    shader.h \
    Trackball.h \
    TransferFunctionControlPoint.h \
    imageloader.h \
    learnopengl/camera.h \
    #learnopengl/filesystem.h \
    #learnopengl/mesh.h \
    #learnopengl/model.h \
    learnopengl/shader.h \
    #learnopengl/shader_m.h \
    #learnopengl/shader_s.h

FORMS    += mainwindow.ui

FreeGlutDir=F:/opengl/freeglut-3.0.0/qt5.3/lib-release
GlewDir = F:/opengl/glew-2.1.0/qt5.3/lib-release
GLMDir = F:/opengl/glm-0.9.9.4
SFMLDir = F:/opengl/SFML-master/qt5.3/lib-release
GLADDir = F:/opengl/glad/qt5.3/lib-release
STDDir = F:/vulkan/stb
GLFWDir = F:/opengl/glfw-3.2.1/qt5.3/lib-release
SOILDir = F:/opengl/Simple_OpenGL_Image_Library/qt5.3
INCLUDEPATH += $${FreeGlutDir}/include
INCLUDEPATH += $${GlewDir}/include
INCLUDEPATH += $${GLMDir}/
INCLUDEPATH += $${SFMLDir}/include
INCLUDEPATH += $${GLADDir}/include
INCLUDEPATH += $${STDDir}/
INCLUDEPATH += $${GLFWDir}/include
INCLUDEPATH += $${SOILDir}/include

LIBS += -L$${FreeGlutDir}/lib libfreeglut
LIBS += -L$${GlewDir}/lib libglew32
LIBS += -L$${SFMLDir}/lib \
        -lsfml-graphics \
        -lsfml-system \
        -lsfml-window
LIBS += -L$${GLFWDir}/lib libglfw3
LIBS += -L$${GLADDir}/lib libglad
LIBS += -L$${SOILDir}/lib libSOIL
LIBS += -lopengl32 -lwinmm -lgdi32 -lm -lglu32
