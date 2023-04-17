#include "../../include/interface/welcWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WelcWindow welc;
    welc.show();
//    Bubble b;
//    b.show();
    return QApplication::exec();
}
