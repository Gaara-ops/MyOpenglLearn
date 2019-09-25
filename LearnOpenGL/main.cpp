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

/// <summary>
/// 使用Monte-Carlo发探测主元素
/// </summary>
/// <param name="elements">所有元素</param>
/// <param name="N">阈值</param>
/// <returns>是否存在主元素</returns>
bool DetectPrincipalElement(QList<int> elements,int N)
{
//    srand((unsigned)time(NULL));//确保每次生成的随机数不一样
//    float randrange[2]={0,elements.size()-1};

    bool result = false;
    for (int i = 0; i <= N; i++)
    {
        int index = random(elements.size()); //产生elements.size()以内的随机数
        int element = elements[(int)index];
        int count = 0;
        for (int j = 0; j < elements.size(); j++)
        {
            if (element == elements[j])
            {
                count++;
            }
        }
        if (count >= elements.size() / 2)
        {
            result = true;
            break;
        }
    }
    return result;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.SetArgInfo(argc,argv);
    w.show();

    /**测试数组中是否有主元素(主元素表述在数组中出现超过半数)
    int arr[] = {4, 5, 8, 1, 8, 4, 9, 2, 2, 2, 2, 2, 5, 7, 8, 2, 2, 2, 2, 2,
                 1, 0, 9, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 4, 7, 8, 2, 2, 2,
                 2, 2, 0, 1, 2, 2, 2, 2, 2, 1};
    QList<int> listtmp;
    for(int i=0;i<50;i++){
        listtmp.append(arr[i]);
    }
    srand((int)time(0));
    for(int i=0;i<10;i++){
        qDebug() << DetectPrincipalElement(listtmp,1)
                 << DetectPrincipalElement(listtmp,3)
                 << DetectPrincipalElement(listtmp,10);
    }
    */

    return a.exec();
}
