#include "include/DataBaseInterface.hpp"
#include <sqlite3.h>
#include <iostream>

namespace database_interface {

void chars_to_string(char *chr, std::string &str){
    while (chr != nullptr && *chr != '\0'){
        str += *chr;
        chr++;
    }
}


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
Status SQL_BDInterface::add_user(User &user) {
    std::string sql =
        "INSERT INTO Users (Name, Surname, Login, PasswordHash) VALUES ('";
    sql += user.m_name + "', '";
    sql += user.m_surname + "', '";
    sql += user.m_login + "', '";
    sql += user.m_password_hash + "')";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);

    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(exit == SQLITE_OK, "Problem in ADD Used.\nMessage: " + string_message + "\n SQL command: " + sql + "\n");
}

//Status SQL_BDInterface::change_user(const User &new_user);
Status SQL_BDInterface::get_user_log_pas(
    User &user
){
    std::string sql =
        "SELECT id, Name, Surname FROM Users WHERE Login='";
    sql += user.m_login + "' AND  PasswordHash='";
    sql += user.m_password_hash + "'";
    char *message_error;
    std::string string_message;
    User::m_edit_user = &user;
    int exit = sqlite3_exec(m_bd, sql.c_str(), User::callback, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(exit == SQLITE_OK, "Problem in ADD Used.\nMessage: " + string_message + "\n SQL command: " + sql + "\n");
}
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