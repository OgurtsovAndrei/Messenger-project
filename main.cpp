//#include "mainwindow.h"
#include "welcWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WelcWindow welc;
    welc.show();
//    MainWindow w;
//    w.show();
    return a.exec();
}
