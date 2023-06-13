#include "interface/welcWindow.h"
#include "Net/NetClient.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Net::Client::Client client("localhost", "12345");
    client.make_secure_connection();
    WelcWindow welcome(&client);
    welcome.show();
    return QApplication::exec();
}
