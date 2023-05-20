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
    database_interface::Dialog dia(4);
    bd.get_dialog_by_id(dia);
    std::cout << "<" << dia.m_dialog_id << " " << dia.m_name << ' ' << dia.m_is_group << '\n';
    std::cout << bd.close().correct() << '\n';
    return 0;
}