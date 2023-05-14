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
    std::string sql = "INSERT INTO Users (Name, Surname, Login, PasswordHash) VALUES ('q', 'q', 'q2', 'ok');";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(bd.m_bd, sql.c_str(), NULL, 0, &message_error);
    std::cout << (exit == SQLITE_OK) << ' ' << message_error << '\n';
    sqlite3_free(message_error);
    std::cout << bd.close().correct() << '\n';
    return (0);
}