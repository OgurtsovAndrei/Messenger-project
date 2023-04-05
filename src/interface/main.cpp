//#include "mainwindow.h"
#include <QApplication>
#include "../../include/interface/welcWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WelcWindow welc;
    welc.show();
//    MainWindow w;
//    w.show();
    return a.exec();
}
