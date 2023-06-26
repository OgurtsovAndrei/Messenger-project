#include <botan/argon2.h>
#include <botan/auto_rng.h>
#include <botan/rng.h>
#include <botan/system_rng.h>
#include <sqlite3.h>
#include <ctime>
#include <iostream>
#include <string>
#include "database/DataBaseInterface.hpp"

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
    exit = sqlite3_open("./../bd/ServerDataBase.db", &m_bd);
    return Status(exit == SQLITE_OK, "Problem in database open\n");
}

Status SQL_BDInterface::close() {
    int exit = 0;
    exit = sqlite3_close(m_bd);
    return Status(exit == SQLITE_OK, "Problem in database close\n");
}

std::string make_hash(const std::string &password) {
    std::unique_ptr<Botan::RandomNumberGenerator> rng;
#if defined(BOTAN_HAS_SYSTEM_RNG)
    rng.reset(new Botan::System_RNG);
#else
    rng.reset(new Botan::AutoSeeded_RNG);
#endif
    return Botan::argon2_generate_pwhash(
        password.c_str(), password.size(), *(rng.get()), 1, 8192, 1
    );
}

Status check_user_id(const User &user, const std::string &where) {
    if (user.m_user_id < 0) {
        return Status(0, "User should have id in " + where);
    }
    return Status(1, "");
}

Status check_user_name(const User &user, const std::string &where) {
    if (user.m_name == "") {
        return Status(0, "User should have name in " + where);
    }
    return Status(1, "");
}

Status check_user_surname(const User &user, const std::string &where) {
    if (user.m_surname == "") {
        return Status(0, "User should have surname in " + where);
    }
    return Status(1, "");
}

Status check_user_login(const User &user, const std::string &where) {
    if (user.m_login == "") {
        return Status(0, "User should have login in " + where);
    }
    return Status(1, "");
}

Status check_user_password_hash(const User &user, const std::string &where) {
    if (user.m_password_hash == "") {
        return Status(0, "User should have password hash in " + where);
    }
    return Status(1, "");
}

Status check_message_id(const Message &message, const std::string &where) {
    if (message.m_message_id < 0) {
        return Status(0, "Message should have id in " + where);
    }
    return Status(1, "");
}

Status check_dialog_id(const Dialog &dialog, const std::string &where) {
    if (dialog.m_dialog_id < 0) {
        return Status(0, "Dialog should have id in " + where);
    }
    return Status(1, "");
}

// User
Status SQL_BDInterface::make_user(User &user) {
    if (Status user_params = check_user_name(user, "MAKE user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params = check_user_surname(user, "MAKE user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params = check_user_login(user, "MAKE user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params = check_user_password_hash(user, "MAKE user");
        !user_params.correct()) {
        return user_params;
    }
    database_interface::User user_log_exist(user.m_login);
    if (get_user_id_by_log(user_log_exist)) {
        return Status(
            false, "Problem in MAKE User.\nMessage: login is already taken\n"
        );
    }
    std::string sql =
        "INSERT INTO Users (Name, Surname, Login, PasswordHash) VALUES ('";
    sql += user.m_name + "', '";
    sql += user.m_surname + "', '";
    sql += user.m_login + "', '";
    sql += make_hash(user.m_password_hash) + "');";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit == SQLITE_OK) {
        Status select = get_user_by_log_pas(user);
        return Status(
            select.correct() && user.m_user_id != -1,
            "Problem in MAKE User in SELECT User.\nMessage: " + select.message()
        );
    }
    return Status(
        exit == SQLITE_OK, "Problem in MAKE User.\nMessage: " + string_message +
                               "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::change_user(const User &new_user) {
    if (Status user_params = check_user_id(new_user, "CHANGE user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params = check_user_name(new_user, "CHANGE user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params = check_user_surname(new_user, "CHANGE user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params = check_user_login(new_user, "CHANGE user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params = check_user_password_hash(new_user, "CHANGE user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql =
        "REPLACE INTO Users (id, Name, Surname, Login, PasswordHash, "
        "Encryption) VALUES(";
    sql += std::to_string(new_user.m_user_id) + ", '";
    sql += new_user.m_name + "', '";
    sql += new_user.m_surname + "', '";
    sql += new_user.m_login + "', '";
    sql += make_hash(new_user.m_password_hash) + "', '";
    sql += std::to_string(new_user.m_encryption) + "');";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in CHANGE User.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_by_id(User &user) {
    if (Status user_params = check_user_id(user, "get_user_by_id user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql = "SELECT id, Name, Surname FROM Users WHERE id=";
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
        exit == SQLITE_OK && user.m_user_id != -1,
        "Problem in GET User by id.\nMessage: " + string_message +
            "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_by_log_pas(User &user) {
    if (Status user_params = check_user_login(user, "get_user_by_log_pas user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params =
            check_user_password_hash(user, "get_user_by_log_pas user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql =
        "SELECT id, Name, Surname, PasswordHash, Encryption FROM Users WHERE "
        "Login='";
    sql += user.m_login + "';";
    char *message_error;
    std::string string_message;
    User tmp_user;
    User::m_edit_user = &tmp_user;
    int exit = sqlite3_exec(
        m_bd, sql.c_str(), User::get_all_params, 0, &message_error
    );
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    User::m_edit_user = nullptr;
    if (exit == SQLITE_OK && tmp_user.m_user_id != -1 &&
        Botan::argon2_check_pwhash(
            user.m_password_hash.c_str(), user.m_password_hash.size(),
            tmp_user.m_password_hash
        )) {
        user.m_user_id = tmp_user.m_user_id;
        user.m_name = tmp_user.m_name;
        user.m_surname = tmp_user.m_surname;
        user.m_encryption = tmp_user.m_encryption;
    }
    return Status(
        exit == SQLITE_OK && user.m_user_id != -1,
        "Problem in GET User.\nMessage: " + string_message +
            "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_id_by_log(User &user) {
    if (Status user_params = check_user_login(user, "get_user_id_by_log user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql = "SELECT id, Name, Surname FROM Users WHERE Login='";
    sql += user.m_login + "';";
    char *message_error;
    std::string string_message;
    User::m_edit_user = &user;
    std::cout << sql << '\n';
    int exit =
        sqlite3_exec(m_bd, sql.c_str(), User::callback, 0, &message_error);
    User::m_edit_user = nullptr;
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK && user.m_user_id != -1,
        "Problem in GET User id by login.\nMessage: " + string_message +
            "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_log_by_id(User &user) {
    if (Status user_params = check_user_id(user, "get_user_log_by_id user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql = "SELECT login FROM Users WHERE id=";
    sql += std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    User tmp_user;
    User::m_edit_user = &tmp_user;
    int exit =
        sqlite3_exec(m_bd, sql.c_str(), User::get_login, 0, &message_error);
    User::m_edit_user = nullptr;
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit == SQLITE_OK && user.m_user_id != -1) {
        std::cout << "OK in db"
                  << " ";
        user.m_login = tmp_user.m_login;
    }
    std::cout << user.m_login << "\n";
    return Status(
        exit == SQLITE_OK && user.m_user_id != -1,
        "Problem in GET User login by id.\nMessage: " + string_message +
            "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_encryption_name_by_id(
    int encryption_id,
    std::string &encryption_name
) {
    std::string sql = "SELECT Encryption FROM Encryptions WHERE id=";
    sql += std::to_string(encryption_id) + ";";
    char *message_error;
    std::string string_message;
    User::m_encryption_name = &encryption_name;
    int exit = sqlite3_exec(
        m_bd, sql.c_str(), User::callback_for_encryption_name, 0, &message_error
    );
    User::m_encryption_name = nullptr;
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in GET Encryption.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_encryption_pairs_id_name(
    std::vector<std::pair<int, std::string>> &encryption_pair_id_name
) {
    std::string sql = "SELECT id, Encryption FROM Encryptions;";
    char *message_error;
    std::string string_message;
    User::m_encryption_pair_id_name = &encryption_pair_id_name;
    int exit = sqlite3_exec(
        m_bd, sql.c_str(), User::User::callback_for_all_encryption_names, 0,
        &message_error
    );
    User::m_encryption_pair_id_name = nullptr;
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK && encryption_pair_id_name.size() > 0,
        "Problem in GET Encryption.\nMessage: " + string_message +
            "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user(const User &user) {
    if (Status user_params = check_user_id(user, "del_user user");
        !user_params.correct()) {
        return user_params;
    }
    Status res = del_user_dialogs(user);
    if (!res.correct()) {
        return Status(
            false, "Can not DEL User Dialogs. It return:\n" + res.message()
        );
    }
    res = del_user_requests(user);
    if (!res.correct()) {
        return Status(
            false, "Can not DEL User Requests. It return:\n" + res.message()
        );
    }
    res = del_user_messages(user);
    if (!res.correct()) {
        return Status(
            false, "Can not DEL User Messages. It return:\n" + res.message()
        );
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

Status SQL_BDInterface::del_user_dialogs(const User &user) {
    if (Status user_params = check_user_id(user, "del_user_dialogs user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql = "DELETE FROM UsersAndDialogs WHERE ";
    sql += "UserId=" + std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL User Dialogs.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user_messages(const User &user) {
    if (Status user_params = check_user_id(user, "del_user_messages user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql = "DELETE FROM Messages WHERE ";
    sql += "UserId=" + std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL User Messages.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user_requests(const User &user) {
    if (Status user_params = check_user_id(user, "del_user_requests user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql = "DELETE FROM RequestForDialog WHERE ";
    sql += "FromUserId=" + std::to_string(user.m_user_id) + " OR ";
    sql += "ToUserId=" + std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL User Requests.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::make_dialog_request(
    const User &from_user,
    const User &to_user
) {
    if (Status user_params =
            check_user_id(from_user, "make_dialog_request from_user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params =
            check_user_id(to_user, "make_dialog_request to_user");
        !user_params.correct()) {
        return user_params;
    }
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
        exit == SQLITE_OK, "Problem in MAKE DIALOG REQUEST.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_user_dialog_requests(
    const User &user,
    std::vector<User> &requests
) {
    if (Status user_params =
            check_user_id(user, "get_user_dialog_requests user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql =
        "SELECT Users.id, Users.Name, Users.Surname FROM RequestForDialog "
        "INNER JOIN Users ON FromUserId = Users.id WHERE ToUserId=";
    sql += std::to_string(user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    requests.clear();
    User::m_requests = &requests;
    int exit = sqlite3_exec(
        m_bd, sql.c_str(), User::request_callback, 0, &message_error
    );
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    User::m_requests = nullptr;
    return Status(
        exit == SQLITE_OK, "Problem in GET n user requests.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::close_dialog_request(
    const User &from_user,
    const User &to_user
) {
    if (Status user_params =
            check_user_id(from_user, "close_dialog_request from_user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status user_params =
            check_user_id(to_user, "close_dialog_request to_user");
        !user_params.correct()) {
        return user_params;
    }
    std::string sql = "DELETE FROM Users WHERE ";
    sql += "FromUserId=" + std::to_string(from_user.m_user_id) + " AND ";
    sql += "ToUserId=" + std::to_string(to_user.m_user_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL DIALOG REQUEST.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::make_dialog(Dialog &dialog) {
    std::string sql =
        "INSERT INTO Dialogs (Name, DateTime, OwnerId, IsGroup) VALUES ('";
    sql += dialog.m_name + "', ";
    sql += std::to_string(time(NULL)) + ", ";
    sql += std::to_string(dialog.m_owner_id) + ", ";
    sql += std::to_string(dialog.m_is_group) + ");";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit != SQLITE_OK) {
        return Status(
            exit == SQLITE_OK,
            "Problem in MAKE Dialog.\nMessage: " + string_message +
                "\n SQL command: " + sql + "\n"
        );
    }
    sql = "SELECT last_insert_rowid();";
    message_error = nullptr;
    string_message = "";
    exit = sqlite3_exec(
        m_bd, sql.c_str(), SQL_BDInterface::get_last_insert_id, 0,
        &message_error
    );
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit != SQLITE_OK) {
        return Status(
            exit == SQLITE_OK && dialog.m_dialog_id != -1,
            "Problem in MAKE Dialog in find id.\nMessage: " + string_message +
                "\n SQL command: " + sql + "\n"
        );
    }
    dialog.m_dialog_id = SQL_BDInterface::last_insert_id;
    return Status(true);
}

Status SQL_BDInterface::change_dialog(const Dialog &new_dialog) {
    std::string sql =
        "REPLACE INTO Dialogs (id, Name, DateTime, OwnerId, IsGroup) VALUES(";
    sql += std::to_string(new_dialog.m_dialog_id) + ", '";
    sql += new_dialog.m_name + "', ";
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
                               string_message + "\n SQL command: " + sql + "\n"
    );
};

Status SQL_BDInterface::add_user_to_dialog(const User &user, Dialog &dialog) {
    if (Status user_params = check_user_id(user, "add_user_to_dialog user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status dialog_params =
            check_dialog_id(dialog, "add_user_to_dialog user");
        !dialog_params.correct()) {
        return dialog_params;
    }
    Status update = update_dialog_time(dialog);
    if (!update.correct()) {
        return Status(
            false, "Problem in UPDATE in ADD User to Dialog.\nMessage: " +
                       update.message()
        );
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
        exit == SQLITE_OK, "Problem in ADD User to Dialog.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_user_from_dialog(const User &user, Dialog &dialog) {
    if (Status user_params = check_user_id(user, "del_user_from_dialog user");
        !user_params.correct()) {
        return user_params;
    }
    if (Status dialog_params =
            check_dialog_id(dialog, "del_user_from_dialog user");
        !dialog_params.correct()) {
        return dialog_params;
    }
    Status update = update_dialog_time(dialog);
    if (!update.correct()) {
        return Status(
            false, "Problem in UPDATE in DEL User to Dialog.\nMessage: " +
                       update.message()
        );
    }
    std::string sql = "DELETE FROM UsersAndDialogs WHERE ";
    sql += "UserId=" + std::to_string(user.m_user_id) + " AND ";
    sql += "DialogId=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL User from Dialog.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_dialog_by_id(Dialog &dialog) {
    if (Status dialog_params = check_dialog_id(dialog, "get_dialog_by_id user");
        !dialog_params.correct()) {
        return dialog_params;
    }
    std::string sql =
        "SELECT id, Name, DateTime, OwnerId, IsGroup FROM Dialogs WHERE id=";
    sql += std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    Dialog::m_edit_dialog = &dialog;
    int exit = sqlite3_exec(
        m_bd, sql.c_str(), Dialog::callback_get_one_dialog, 0, &message_error
    );
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    Dialog::m_edit_dialog = nullptr;
    return Status(
        exit == SQLITE_OK, "Problem in GET Dialog by id.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::add_users_to_dialog(
    const std::vector<User> &users,
    Dialog &dialog
) {
    for (auto &user : users) {
        Status exit = add_user_to_dialog(user, dialog);
        if (!exit.correct()) {
            return Status(
                false,
                "Problem in ADD Users to Dialog.\n One of 'add user' return "
                "this message: " +
                    exit.message() + "\n"
            );
        }
    }
    return Status(true, "Problem in ADD Users to Dialog.\n");
}

Status SQL_BDInterface::get_users_in_dialog(
    const Dialog &dialog,
    std::vector<User> &users
) {
    if (Status dialog_params =
            check_dialog_id(dialog, "get_users_in_dialog dialog");
        !dialog_params.correct()) {
        return dialog_params;
    }
    std::string sql = "SELECT UserId FROM UsersAndDialogs WHERE DialogId=";
    sql += std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    users.clear();
    Dialog::m_users = &users;
    int exit = sqlite3_exec(
        m_bd, sql.c_str(), Dialog::callback_get_dialog_users, 0, &message_error
    );
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    Dialog::m_users = nullptr;
    return Status(
        exit == SQLITE_OK,
        "Problem in GET n users dialogs by time.\nMessage: " + string_message +
            "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::update_dialog_time(const Dialog &dialog) {
    if (Status dialog_params =
            check_dialog_id(dialog, "update_dialog_time dialog");
        !dialog_params.correct()) {
        return dialog_params;
    }
    std::string sql = "UPDATE Dialogs\nSET DateTime=";
    sql += std::to_string(time(NULL)) + "\n";
    sql += "WHERE id=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in UPDATE Dialog time.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_dialog(const Dialog &dialog) {
    if (Status dialog_params = check_dialog_id(dialog, "del_dialog dialog");
        !dialog_params.correct()) {
        return dialog_params;
    }
    Status res = del_all_massages_in_dialog(dialog);
    if (!res.correct()) {
        return Status(
            false, "Can not del dialog messages. It return:\n" + res.message()
        );
    }
    res = del_all_users_in_dialog(dialog);
    if (!res.correct()) {
        return Status(
            false, "Can not del dialog users. It return:\n" + res.message()
        );
    }
    std::string sql = "DELETE FROM Dialogs WHERE ";
    sql += "id=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL Dialog.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_all_massages_in_dialog(const Dialog &dialog) {
    if (Status dialog_params =
            check_dialog_id(dialog, "del_all_massages_in_dialog dialog");
        !dialog_params.correct()) {
        return dialog_params;
    }
    std::string sql = "DELETE FROM Messages WHERE ";
    sql += "DialogId=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL Dialog Messages.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_all_users_in_dialog(const Dialog &dialog) {
    if (Status dialog_params =
            check_dialog_id(dialog, "del_all_users_in_dialog dialog");
        !dialog_params.correct()) {
        return dialog_params;
    }
    std::string sql = "DELETE FROM UsersAndDialogs WHERE ";
    sql += "DialogId=" + std::to_string(dialog.m_dialog_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL Dialog Users.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

// Message
Status SQL_BDInterface::make_message(Message &message) {
    Status update = update_dialog_time(Dialog(message.m_dialog_id));
    if (!update.correct()) {
        return Status(
            false,
            "Problem in UPDATE in MAKE Message.\nMessage: " + update.message()
        );
    }
    std::string sql =
        "INSERT INTO Messages (DateTime, Text, File, DialogId, UserId) VALUES "
        "(";
    sql += std::to_string(time(NULL)) + ", ";
    sql += "?, ?, ";
    sql += std::to_string(message.m_dialog_id) + ", ";
    sql += std::to_string(message.m_user_id) + ");";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(m_bd, sql.c_str(), sql.length(), &stmt, nullptr);
    sqlite3_bind_text(
        stmt, 1, message.m_text.c_str(), message.m_text.length(), SQLITE_STATIC
    );
    sqlite3_bind_text(
        stmt, 2, message.m_file_name.c_str(), message.m_file_name.length(),
        SQLITE_STATIC
    );
    int exit = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (exit != SQLITE_DONE) {
        return Status(
            exit == SQLITE_DONE,
            "Problem in MAKE Message.\nMessage: \n SQL command: " + sql + "\n"
        );
    }
    sql = "SELECT last_insert_rowid();";
    char *message_error = nullptr;
    std::string string_message = "";
    exit = sqlite3_exec(
        m_bd, sql.c_str(), SQL_BDInterface::get_last_insert_id, 0,
        &message_error
    );
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    if (exit != SQLITE_OK) {
        return Status(
            exit == SQLITE_OK,
            "Problem in MAKE Message in find id.\nMessage: " + string_message +
                "\n SQL command: " + sql + "\n"
        );
    }
    message.m_message_id = SQL_BDInterface::last_insert_id;
    return Status(true);
}

Status SQL_BDInterface::change_message(const Message &new_message) {
    if (Status message_params =
            check_message_id(new_message, "change_message message");
        !message_params.correct()) {
        return message_params;
    }
    if (Status message_params = check_dialog_id(
            Dialog(new_message.m_dialog_id), "change_message message"
        );
        !message_params.correct()) {
        return message_params;
    }
    if (Status message_params = check_user_id(
            User(new_message.m_user_id), "change_message message"
        );
        !message_params.correct()) {
        return message_params;
    }
    std::string sql =
        "REPLACE INTO Messages (id, DateTime, Text, File, DialogId, UserId) "
        "VALUES(";
    sql += std::to_string(new_message.m_message_id) + ", ";
    sql += std::to_string(new_message.m_date_time) + ", ";
    sql += "?, ?, ";
    sql += std::to_string(new_message.m_dialog_id) + ", ";
    sql += std::to_string(new_message.m_user_id) + ");";
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(m_bd, sql.c_str(), sql.length(), &stmt, nullptr);
    sqlite3_bind_text(
        stmt, 1, new_message.m_text.c_str(), new_message.m_text.length(),
        SQLITE_STATIC
    );
    sqlite3_bind_text(
        stmt, 2, new_message.m_file_name.c_str(),
        new_message.m_file_name.length(), SQLITE_STATIC
    );
    int exit = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return Status(
        exit == SQLITE_DONE,
        "Problem in CHANGE Message.\nMessage: \n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::get_message_by_id(Message &message) {
    if (Status message_params =
            check_message_id(message, "get_message_by_id message");
        !message_params.correct()) {
        return message_params;
    }
    std::string sql =
        "SELECT id, DateTime, Text, File, DialogId, UserId FROM Messages WHERE "
        "id=";
    sql += std::to_string(message.m_message_id) + ";";
    char *message_error;
    std::string string_message;
    Message::m_edit_message = &message;
    int exit = sqlite3_exec(
        m_bd, sql.c_str(), Message::callback_get_message_by_id, 0,
        &message_error
    );
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    Message::m_edit_message = nullptr;
    return Status(
        exit == SQLITE_OK, "Problem in GET Message by id.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

Status SQL_BDInterface::del_message(const Message &message) {
    if (Status message_params =
            check_message_id(message, "del_message message");
        !message_params.correct()) {
        return message_params;
    }
    std::string sql = "DELETE FROM Messages WHERE ";
    sql += "id=" + std::to_string(message.m_message_id) + ";";
    char *message_error;
    std::string string_message;
    int exit = sqlite3_exec(m_bd, sql.c_str(), NULL, 0, &message_error);
    chars_to_string(message_error, string_message);
    sqlite3_free(message_error);
    return Status(
        exit == SQLITE_OK, "Problem in DEL Message.\nMessage: " +
                               string_message + "\n SQL command: " + sql + "\n"
    );
}

}  // namespace database_interface
