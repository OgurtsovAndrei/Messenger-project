#include <sqlite3.h>
#include <iostream>
#include <list>
#include <chrono>
#include <ctime>
#include "DataBaseInterface.hpp"
#include "User.hpp"

int main(int argc, char **argv) {
    database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();
    std::cout << bd.open().correct() << '\n';
    database_interface::Message mes("Jon's strange's people's can't writ't", "fs", 1, 2);
    std::cout << bd.make_message(mes).correct() << " " << bd.make_message(mes).message() << '\n';
    std::cout << bd.close().correct() << '\n';
    return 0;
}