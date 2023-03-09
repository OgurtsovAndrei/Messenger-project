#include "include/DataBaseInterface.hpp"
#include <sqlite3.h>
#include <iostream>

namespace database_interface {

Status SQL_BDInterface::open() {
    int exit = 0;
    exit = sqlite3_open("ServerDataBase.db", &m_bd);
    return Status(exit != SQLITE_OK, "Problem in database open\n");
}

Status SQL_BDInterface::close() {
    int exit = 0;
    exit = sqlite3_close(m_bd);
    // Почитать возврат значения при закрытии бд.
    return Status(exit != SQLITE_OK, "Problem in database close\n");
}

// User
Status SQL_BDInterface::add_user(const User &user);
Status SQL_BDInterface::change_user(const User &new_user);
Status SQL_BDInterface::get_user(std::string login, std::string password_hash, User &user);
Status SQL_BDInterface::del_user(const User &user);
Status SQL_BDInterface::make_dialog_request(const User &from_user, const User &to_user);
Status SQL_BDInterface::close_dilog_request(const User &from_user, const User &to_user);

}  // namespace database_interface