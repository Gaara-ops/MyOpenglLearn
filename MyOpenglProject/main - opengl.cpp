#include "mainwindow.h"
#include <QApplication>
#include <QVector>
#include <QList>
#include <QDebug>

#include <iostream>
#include <stdio.h>

#include<GL/freeglut.h>
void RenderScenceCB(){
    // 清空颜色缓存
    glClear(GL_COLOR_BUFFER_BIT);
    // 交换前后缓存
    glutSwapBuffers();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(300, 300);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("OpenGL Version");

    const GLubyte* name = glGetString(GL_VENDOR); //返回负责当前OpenGL实现厂商的名字
    const GLubyte* biaoshifu = glGetString(GL_RENDERER); //返回一个渲染器标识符，通常是个硬件平台
    const GLubyte* OpenGLVersion = glGetString(GL_VERSION); //返回当前OpenGL实现的版本号
    const GLubyte* gluVersion = gluGetString(GLU_VERSION); //返回当前GLU工具库版本

    printf("OpenGL实现厂商的名字：%s\n", name);
    printf("渲染器标识符：%s\n", biaoshifu);
    printf("OpenGL实现的版本号：%s\n", OpenGLVersion);
    printf("OGLU工具库版本：%s\n", gluVersion);

/*
    // 初始化GLUT
    glutInit(&argc, argv);

    // 显示模式：双缓冲、RGBA
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    // 窗口设置
    glutInitWindowSize(480, 320);      // 窗口尺寸
    glutInitWindowPosition(100, 100);  // 窗口位置
    glutCreateWindow("Tutorial 01");   // 窗口标题

    // 开始渲染
    glutDisplayFunc(RenderScenceCB);

    // 缓存清空后的颜色值
    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);

    // 通知开始GLUT的内部循环
    glutMainLoop();
*/
    return a.exec();
}
