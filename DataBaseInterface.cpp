#include "include/DataBaseInterface.hpp"
#include <sqlite3.h>
#include <iostream>

namespace database_interface {

Status SQL_BDInterface::open() {
    int exit = 0;
    exit = sqlite3_open("bd/ServerDataBase.db", &m_bd);
    return Status(exit == SQLITE_OK, "Problem in database open\n");
}

Status SQL_BDInterface::close() {
    int exit = 0;
    exit = sqlite3_close(m_bd);
    // Почитать возврат значения при закрытии бд.
    return Status(exit == SQLITE_OK, "Problem in database close\n");
}

// User
Status SQL_BDInterface::add_user(const User &user) {
    std::string sql =
        "INSERT INTO Users (id, Name, Surname, Login, PasswordHash) VALUES (";
    sql += std::to_string(user.m_user_id) + ", '";
    sql += user.m_name + "', '";
    sql += user.m_surname + "', '";
    sql += user.m_login + "', '";
    sql += user.m_password_hash + "'); ";
    std::cout << sql << "\n";
    char *messaggeError;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &messaggeError);
    sqlite3_free(messaggeError);
    return Status(exit == SQLITE_OK, "Problem in ADD Used.\nMessage: \n");
}

//Status SQL_BDInterface::change_user(const User &new_user);
//Status SQL_BDInterface::get_user(
//    std::string login,
//    std::string password_hash,
//    User &user
//);
//Status SQL_BDInterface::del_user(const User &user);
//Status SQL_BDInterface::make_dialog_request(
//    const User &from_user,
//    const User &to_user
//);
//Status SQL_BDInterface::close_dilog_request(
//    const User &from_user,
//    const User &to_user
//);

}  // namespace database_interface