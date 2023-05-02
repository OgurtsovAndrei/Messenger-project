#include <sqlite3.h>
#include <iostream>
#include <ctime>
#include "../../../include/database/DataBaseInterface.hpp"

namespace database_interface {

void chars_to_string(char *chr, std::string &str) {
    while (chr != nullptr && *chr != '\0') {
        str += *chr;
        chr++;
    }
}

int SQL_BDInterface::last_insert_id = -1;

Status SQL_BDInterface::open() {
    int exit = 0;
//    exit = sqlite3_open("./../bd/ServerDataBase.db", &m_bd);
    exit = sqlite3_open("./../../../bd/ServerDataBase.db", &m_bd);
//    exit = sqlite3_open("/Users/arina/hse/project/Messenger-project/bd/ServerDataBase.db", &m_bd);
    return Status(exit == SQLITE_OK, "Problem in database open\n");
}

Status SQL_BDInterface::close() {
    int exit = 0;
    exit = sqlite3_close(m_bd);
    return Status(exit == SQLITE_OK, "Problem in database close\n");
}

// User
Status SQL_BDInterface::make_user(User &user) {
    std::string sql =
        "INSERT INTO Users (Name, Surname, Login, PasswordHash) VALUES ('";
    sql += user.m_name + "', '";
    sql += user.m_surname + "', '";
    sql += user.m_login + "', '";
    sql += user.m_password_hash + "');";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit == SQLITE_OK) {
        Status select = get_user_by_log_pas(user);
        return Status(
            select.correct(),
            "Problem in MAKE User in SELECT User.\nMessage: " + select.message()
        );
    }
    return Status(
        exit == SQLITE_OK, "Problem in MAKE User.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::change_user(const User &new_user) {
     std::string sql =
         "REPLACE INTO Users (id, Name, Surname, Login, PasswordHash) VALUES(";
     sql += std::to_string(new_user.m_user_id) + ", '";
     sql += new_user.m_name + "', '";
     sql += new_user.m_surname + "', '";
     sql += new_user.m_login + "', '";
     sql += new_user.m_password_hash + "');";
     char *message_error;
     std::string string_message;
     int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
     chars_to_string(message_error, string_message);
     sqlite3_free(message_error);
     return Status(
         exit == SQLITE_OK, "Problem in CHANGE User.\nMessage: " +
                                string_message + "\n SQL command: " + sql +
                                "\n"
     );
}

Status SQL_BDInterface::get_user_by_id(User &user){
    std::string sql = "SELECT Name, Surname FROM Users WHERE id=";
    sql += std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    User::m_edit_user = &user;
    int exit =
            sqlite3_exec(m_bd, sql.c_str(), User::callback, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    User::m_edit_user = nullptr;
    return Status(
            exit == SQLITE_OK, "Problem in GET User by id.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_by_log_pas(User &user) {
    std::string sql = "SELECT id, Name, Surname FROM Users WHERE Login='";
    sql += user.m_login + "' AND  PasswordHash='";
    sql += user.m_password_hash + "';";
    char *message_error;
    std::string string_message;
    User::m_edit_user = &user;
    int exit =
        sqlite3_exec(m_bd, sql.c_str(), User::callback, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    User::m_edit_user = nullptr;
    return Status(
        exit == SQLITE_OK && user.m_user_id != -1, "Problem in GET User.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_id_by_log(User &user){
    std::string sql = "SELECT id, Name, Surname FROM Users WHERE Login='";
    sql += user.m_login + "';";
    char *message_error;
    std::string string_message;
    User::m_edit_user = &user;
    int exit =
            sqlite3_exec(m_bd, sql.c_str(), User::callback, 0, &message_error);
    User::m_edit_user = nullptr;
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(exit == SQLITE_OK, "Problem in GET User id by login.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user(const User &user) {
    Status res = del_user_dialogs(user);
    if (!res.correct()){
        return Status(false, "Can not DEL User Dialogs. It return:\n" + res.message());
    }
    res = del_user_requests(user);
    if (!res.correct()){
        return Status(false, "Can not DEL User Requests. It return:\n" + res.message());
    }
    res = del_user_messages(user);
    if (!res.correct()){
        return Status(false, "Can not DEL User Messages. It return:\n" + res.message());
    }
    std::string sql = "DELETE FROM Users WHERE ";
    sql += "id=" + std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL User.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user_dialogs(const User &user){
    std::string sql = "DELETE FROM UsersAndDialogs WHERE ";
    sql += "UserId=" + std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL User Dialogs.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user_messages(const User &user){
    std::string sql = "DELETE FROM Messages WHERE ";
    sql += "UserId=" + std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL User Messages.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user_requests(const User &user){
    std::string sql = "DELETE FROM RequestForDialog WHERE ";
    sql += "FromUserId=" + std::to_string(user.m_user_id) + " OR ";
    sql += "ToUserId=" + std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL User Requests.\nMessage: " + string_message +
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

Status SQL_BDInterface::get_user_dialog_requests(const User &user, std::vector<User> &requests){
    std::string sql = "SELECT Users.id, Users.Name, Users.Surname FROM RequestForDialog INNER JOIN Users ON FromUserId = Users.id WHERE ToUserId=";
    sql += std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    requests.clear();
    User::m_requests = &requests;
    int exit =
            sqlite3_exec(m_bd, sql.c_str(), User::request_callback, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    User::m_requests = nullptr;
    return Status(
            exit == SQLITE_OK, "Problem in GET n user requests.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::close_dialog_request(
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
            "INSERT INTO Dialogs (Name, Encryption, DateTime, OwnerId, IsGroup) VALUES ('";
    sql += dialog.m_name + "', '";
    sql += dialog.m_encryption + "', ";
    sql += std::to_string(time(NULL)) + ", ";
    sql += std::to_string(dialog.m_owner_id) + ", ";
    sql += std::to_string(dialog.m_is_group) + ");";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit != SQLITE_OK){
        return Status(
                exit == SQLITE_OK, "Problem in MAKE Dialog.\nMessage: " + string_message +
                                   "\n SQL command: " + sql + "\n"
        );
    }
    sql = "SELECT last_insert_rowid();";
    message_error= nullptr;
    string_message="";
    exit = sqlite3_exec(m_bd, sql.c_str(), SQL_BDInterface::get_last_insert_id, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit != SQLITE_OK){
        return Status(
                exit == SQLITE_OK, "Problem in MAKE Dialog in find id.\nMessage: " + string_message +
                                   "\n SQL command: " + sql + "\n"
        );
    }
    dialog.m_dialog_id = SQL_BDInterface::last_insert_id;
    return Status(true);
}

Status SQL_BDInterface::change_dialog(const Dialog &new_dialog){
    std::string sql =
            "REPLACE INTO Dialogs (id, Name, Encryption, DateTime, OwnerId, IsGroup) VALUES(";
    sql += std::to_string(new_dialog.m_dialog_id) + ", '";
    sql += new_dialog.m_name + "', '";
    sql += new_dialog.m_encryption + "', ";
    sql += std::to_string(time(NULL)) + ", ";
    sql += std::to_string(new_dialog.m_owner_id) + ", ";
    sql += std::to_string(new_dialog.m_is_group) + ");";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in CHANGE Dialog.\nMessage: " +
                               string_message + "\n SQL command: " + sql +
                               "\n"
    );
};

Status SQL_BDInterface::add_user_to_dialog(const User &user, Dialog &dialog){
    Status update = update_dialog_time(dialog);
    if (!update.correct()){
        return Status(false, "Problem in UPDATE in ADD User to Dialog.\nMessage: " + update.message());
    }
    std::string sql = "INSERT INTO UsersAndDialogs (UserId, DialogId) VALUES (";
    sql += std::to_string(user.m_user_id) + ", ";
    sql += std::to_string(dialog.m_dialog_id) + ");";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in ADD User to Dialog.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::add_users_to_dialog(const std::vector<User> &users, Dialog &dialog){
    for (auto &user : users){
        Status exit = add_user_to_dialog(user, dialog);
        if (!exit.correct()){
            return Status(false, "Problem in ADD Users to Dialog.\n One of 'add user' return this message: " + exit.message() + "\n");
        }
    }
    return Status(true, "Problem in ADD Users to Dialog.\n");
}

Status SQL_BDInterface::get_users_in_dialog(const Dialog &dialog, std::vector<User> &users){
    std::string sql = "SELECT UserId FROM UsersAndDialogs WHERE DialogId=";
    sql += std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    users.clear();
    Dialog::m_users = &users;
    int exit = sqlite3_exec(m_bd, sql.c_str(), Dialog::callback_get_dialog_users, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    Dialog::m_users = nullptr;
    return Status(
            exit == SQLITE_OK, "Problem in GET n users dialogs by time.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::update_dialog_time(const Dialog &dialog){
    std::string sql = "UPDATE Dialogs\nSET DateTime=";
    sql += std::to_string(dialog.m_dialog_id) + "\n";
    sql += "WHERE id=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in UPDATE Dialog time.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_dialog(const Dialog &dialog){
    Status res = del_all_massages_in_dialog(dialog);
    if (!res.correct()){
        return Status(false, "Can not del dialog messages. It return:\n" + res.message());
    }
    res = del_all_users_in_dialog(dialog);
    if (!res.correct()){
        return Status(false, "Can not del dialog users. It return:\n" + res.message());
    }
    std::string sql = "DELETE FROM Dialogs WHERE ";
    sql += "id=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL Dialog.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_all_massages_in_dialog(const Dialog &dialog){
    std::string sql = "DELETE FROM Messages WHERE ";
    sql += "DialogId=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL Dialog Messages.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_all_users_in_dialog(const Dialog &dialog){
    std::string sql = "DELETE FROM UsersAndDialogs WHERE ";
    sql += "DialogId=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL Dialog Users.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

// Message
Status SQL_BDInterface::make_message(Message &message){
        Status update = update_dialog_time(Dialog(message.m_dialog_id));
        if (!update.correct()){
            return Status(false, "Problem in UPDATE in MAKE Message.\nMessage: " + update.message());
        }
        std::string sql =
                "INSERT INTO Messages (DateTime, Text, File, DialogId, UserId) VALUES (";
        sql += std::to_string(time(NULL)) + ", '";
        sql += message.m_text + "', '";
        sql += message.m_file_path + "', ";
        sql += std::to_string(message.m_dialog_id) + ", ";
        sql += std::to_string(message.m_user_id) + ");";
        char *message_error;
        std::string string_message;
        int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
        chars_to_string(message_error, string_message);
        sqlite3_free(message_error);
        return Status(
                exit == SQLITE_OK, "Problem in MAKE Message.\nMessage: " + string_message +
                                   "\n SQL command: " + sql + "\n"
        );
}

Status SQL_BDInterface::change_message(const Message &new_message){
    std::string sql =
            "REPLACE INTO Messages (id, DateTime, Text, File, DialogId, UserId) VALUES(";
    sql += std::to_string(new_message.m_message_id) + ", '";
    sql += std::to_string(new_message.m_date_time) + ", '";
    sql += new_message.m_text + "', '";
    sql += new_message.m_file_path + "', ";
    sql += std::to_string(new_message.m_dialog_id) + ", ";
    sql += std::to_string(new_message.m_user_id) + ");";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in CHANGE Message.\nMessage: " +
                               string_message + "\n SQL command: " + sql +
                               "\n"
    );
}

Status SQL_BDInterface::get_message_by_id(Message &message) {
    std::string sql = "SELECT id, DateTime, Text, File, UserId FROM Messages WHERE id=";
    sql += std::to_string(message.m_message_id) + ";";
    char *message_error;
    std::string string_message;
    Message::m_edit_message = &message;
    int exit =
            sqlite3_exec(m_bd, sql.c_str(), Message::callback_get_message_by_id, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    Message::m_edit_message = nullptr;
    return Status(
            exit == SQLITE_OK, "Problem in GET Message by id.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_message(const Message &message){
    std::string sql = "DELETE FROM Messages WHERE ";
    sql += "id=" + std::to_string(message.m_message_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
            exit == SQLITE_OK, "Problem in DEL Message.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}


}  // namespace database_interface