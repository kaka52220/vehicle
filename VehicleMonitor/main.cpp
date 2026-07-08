#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
int main(int argc, char *argv[])
{
    qDebug() << "程序启动";
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    qDebug() << "窗口已显示";
    return a.exec();
}
