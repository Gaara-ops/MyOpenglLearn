#include "mainwindow.h"
#include <QApplication>

#include <QDebug>
#include <QList>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#define random(x) (rand()%x)

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.SetArgInfo(argc,argv);
    w.show();
    return a.exec();
}
