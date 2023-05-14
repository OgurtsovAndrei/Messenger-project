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
    database_interface::User user("q", "q", "q", "q");
    auto exit = bd.make_user(user);
    std::cout << exit.correct() << exit.message() << '\n';
    exit = bd.get_user_by_log_pas(user);
    std::cout << exit.correct() << exit.message() << '\n';
    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << '|' << user.m_login << '|' << user.m_password_hash << ">\n";
    std::cout << bd.close().correct() << '\n';
    return (0);
}