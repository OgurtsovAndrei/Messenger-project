#include <sqlite3.h>
#include <iostream>
#include "include/DataBaseInterface.hpp"

namespace database_interface {

void chars_to_string(char *chr, std::string &str) {
    while (chr != nullptr && *chr != '\0') {
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
    if (exit == SQLITE_OK) {
        Status select = get_user_log_pas(user);
        return Status(
            select.m_correct,
            "Problem in ADD User in SELECT User.\nMessage: " + select.m_message
        );
    }
    return Status(
        exit == SQLITE_OK, "Problem in ADD User.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::change_user(const User &new_user) {
     std::string sql =
         "REPLACE INTO Users (id, Name, Surname, Login, PasswordHash) VALUES
         (";
     sql += std::to_string(new_user.m_user_id) + ", '";
     sql += new_user.m_name + "', '";
     sql += new_user.m_surname + "', '";
     sql += new_user.m_login + "', '";
     sql += new_user.m_password_hash + "')";
     char *message_error;
     std::string string_message;
     int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
     chars_to_string(message_error, string_message);
     sqlite3_free(message_error);
     return Status(
         exit == SQLITE_OK, "Problem in CHANGE Used.\nMessage: " +
                                string_message + "\n SQL command: " + sql +
                                "\n"
     );
}

Status SQL_BDInterface::get_user_by_log_pas(User &user) {
    std::string sql = "SELECT id, Name, Surname FROM Users WHERE Login='";
    sql += user.m_login + "' AND  PasswordHash='";
    sql += user.m_password_hash + "'";
    char *message_error;
    std::string string_message;
    User::m_edit_user = &user;
    int exit =
        sqlite3_exec(m_bd, sql.c_str(), User::callback, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in GET User.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_id_by_log(User &user){
    std::string sql = "SELECT id, Name, Surname FROM Users WHERE Login='";
    sql += user.m_login + "'";
    char *message_error;
    std::string string_message;
    User new_user = User();
    User::m_edit_user = &new_user;
    int exit =
            sqlite3_exec(m_bd, sql.c_str(), User::callback, 0, &message_error);
    if (new_user.m_user_id != -1) {
        user.m_user_id = new_user.m_user_id;
    }
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(exit == SQLITE_OK, "Problem in GET User id.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user(const User &user) {
    std::string sql = "DELETE FROM Users WHERE ";
    sql += "id=" + std::to_string(user.m_user_id) + " AND ";
    sql += "Name='" + user.m_name + "' AND ";
    sql += "Surname='" + user.m_surname + "' AND ";
    sql += "Login='" + user.m_login + "' AND ";
    sql += "PasswordHash='" + user.m_password_hash + "'";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL Used.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::make_dialog_request(
    const User &from_user,
    const User &to_user
) {
    std::string sql =
        "INSERT INTO RequestForDialog (FromUserId, ToUserId) VALUES (";
    sql += std::to_string(from_user.m_user_id) + ", ";
    sql += std::to_string(to_user.m_user_id) + ");";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in MAKE DIALOG REQUEST.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::close_dilog_request(
    const User &from_user,
    const User &to_user
) {
    std::string sql = "DELETE FROM Users WHERE ";
    sql += "FromUserId=" + std::to_string(from_user.m_user_id) + " AND ";
    sql += "ToUserId=" + std::to_string(to_user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL DIALOG REQUEST.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::make_dialog(Dialog &dialog){
    std::string sql =
            "INSERT INTO Dialogs (Name, Encryption, DateTime, Owner, IsGroup) VALUES ('";
    sql += dialog.m_name + "', '";
    sql += dialog.m_encryption + "', '";
    sql += std::to_string(dialog.m_date_time) + "', '";
    sql += std::to_string(dialog.m_owner_id) + "', '";
    sql += std::to_string(dialog.m_i) + "', '";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in MAKE DIALOG REQUEST.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status change_dialog(const Dialog &new_dialog);

Status get_n_users_dialogs_by_time(
        const User &user,
        std::list<Dialog> &next_dialogs,
        int n = 10,
        int last_dialog_date_time = 2121283574
);

Status del_dialog(const Dialog &dialog);


}  // namespace database_interface