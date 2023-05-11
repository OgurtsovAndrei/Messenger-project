#ifndef DATA_BASE_INTERFACE_HPP
#define DATA_BASE_INTERFACE_HPP

#include <sqlite3.h>
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <set>
#include "database/Dialog.hpp"
#include "database/Message.hpp"
#include "Status.hpp"
#include "database/User.hpp"

namespace database_interface {

void chars_to_string(char *chr, std::string &str);

struct BDInterface {
    sqlite3 *m_bd;

    // Work with bd connection
    virtual Status open() = 0;

    virtual Status close() = 0;

    // User
    virtual Status make_user(User &user) = 0;

    virtual Status change_user(const User &new_user) = 0;

    virtual Status get_user_by_log_pas(User &user) = 0;

    virtual Status get_user_id_by_log(User &user) = 0;

    virtual Status del_user(const User &user) = 0;

    virtual Status
    make_dialog_request(const User &from_user, const User &to_user) = 0;

    virtual Status
    get_user_dialog_requests(const User &user, std::vector<User> &requests) = 0;

    virtual Status
    close_dialog_request(const User &from_user, const User &to_user) = 0;

    // Dialog
    virtual Status make_dialog(Dialog &dialog) = 0;

    virtual Status change_dialog(const Dialog &new_dialog) = 0;

    [[maybe_unused]] virtual Status get_n_users_dialogs_by_time(
        const User &user,
        std::list<Dialog> &next_dialogs,
        int n = 10,
        int last_dialog_date_time = 2121283574
    ) = 0;

    virtual Status del_dialog(const Dialog &dialog) = 0;

    // Message
    virtual Status make_message(Message &message) = 0;

    virtual Status change_message(const Message &new_message) = 0;

    virtual Status get_n_dialogs_messages_by_time(
        const Dialog &dialog,
        std::list<Message> &next_messages,
        int n = 10,
        int last_message_date_time = 2121283574
    ) = 0;

    virtual Status del_message(const Message &message) = 0;
};

struct SQL_BDInterface : BDInterface {
    static int last_insert_id;

    static int get_last_insert_id(void *NotUsed, int argc, char **argv, char **azColName){
        assert(argc != 0);
        last_insert_id = std::stoi(argv[0]);
        return 0;
    }

    // Work with bd connection
    Status open() override;

    Status close() override;

    // User
    Status make_user(User &user) override;

    Status change_user(const User &new_user) override;

    Status get_user_by_id(User &user);

    Status get_user_by_log_pas(User &user) override;

    Status get_user_id_by_log(User &user) override;

    Status del_user(const User &user) override;

    Status del_user_dialogs(const User &user);

    Status del_user_messages(const User &user);

    Status del_user_requests(const User &user);

    Status make_dialog_request(const User &from_user, const User &to_user) override;

    Status close_dialog_request(const User &from_user, const User &to_user) override;

    Status get_user_dialog_requests(const User &user, std::vector<User> &requests) override;

    // Dialog
    Status make_dialog(Dialog &dialog) override;

    Status change_dialog(const Dialog &new_dialog) override;

    Status add_user_to_dialog(const User &user, Dialog &dialog);

    Status add_users_to_dialog(const std::vector<User> &users, Dialog &dialog);

    Status get_users_in_dialog(const Dialog &dialog, std::vector<User> &users);

    Status get_n_users_dialogs_by_time(
        const User &user,
        std::list<Dialog> &next_dialogs,
        int n = 10,
        int last_dialog_date_time = 2121283574
    ) override {
        std::string sql = "SELECT Dialogs.id, Name, Encryption, DateTime, OwnerId, IsGroup FROM UsersAndDialogs INNER JOIN Dialogs ON DialogId = Dialogs.id WHERE UserId=";
        sql += std::to_string(user.m_user_id) + " AND DateTime < ";
        sql += std::to_string(last_dialog_date_time) + " ORDER BY DateTime DESC LIMIT ";
        sql += std::to_string(n) + ";";
        char *message_error;
        std::string string_message;
        next_dialogs.clear();
        Dialog::m_dialogs = &next_dialogs;
        int exit =
                sqlite3_exec(m_bd, sql.c_str(), Dialog::callback_get_dialogs, 0, &message_error);
        chars_to_string(message_error, string_message);
        sqlite3_free(message_error);
        Dialog::m_dialogs = nullptr;
        return Status(
                exit == SQLITE_OK, "Problem in GET n users dialogs by time.\nMessage: " + string_message +
                                   "\n SQL command: " + sql + "\n"
        );
    }

    Status update_dialog_time(const Dialog &dialog);

    Status del_dialog(const Dialog &dialog) override;

    Status del_all_massages_in_dialog(const Dialog &dialog);

    Status del_all_users_in_dialog(const Dialog &dialog);

    // Message
    Status make_message(Message &message) override;

    Status change_message(const Message &new_message) override;

    Status get_message_by_id(Message &new_message);

    Status get_n_dialogs_messages_by_time(
        const Dialog &dialog,
        std::list<Message> &next_messages,
        int n = 10,
        int last_message_date_time = 2121283574
    ) override {
        std::string sql = "SELECT id, DateTime, Text, File, UserId FROM Messages WHERE DialogId=";
        sql += std::to_string(dialog.m_dialog_id) + " AND DateTime<";
        sql += std::to_string(last_message_date_time) + " ORDER BY DateTime DESC LIMIT ";
        sql += std::to_string(n) + ";";
        char *message_error;
        std::string string_message;
        next_messages.clear();
        Message::m_message_list = &next_messages;
        int exit =
                sqlite3_exec(m_bd, sql.c_str(), Message::callback_get_message_list, 0, &message_error);
        chars_to_string(message_error, string_message);
        sqlite3_free(message_error);
        Message::m_message_list = nullptr;
        for (auto it = next_messages.begin(); it != next_messages.end(); it++){
            it->m_dialog_id = dialog.m_dialog_id;
        }
        return Status(
                exit == SQLITE_OK, "Problem in GET n users dialogs by time.\nMessage: " + string_message +
                                   "\n SQL command: " + sql + "\n"
        );
    }

    Status del_message(const Message &message) override;
};

struct Mock_BDInterface : BDInterface {
    int last_user_id = 0;
    int last_dialog_id = 0;
    int last_message_id = 0;
    std::map<std::string, User> users;
    std::map<std::string, std::list<User>> all_requests;
    std::map<int, std::list<User>> dialog_users;
    std::list<Dialog> dialogs;
    std::list<Message> messages;

    // Work with bd connection
    Status open() override {
        return Status(true, "Open mock_bd");
    }

    Status close() override {
        return Status(true, "Close mock_bd");
    }

    // User
    Status make_user(User &user) override;

    Status change_user(const User &new_user) override;

    Status get_user_by_log_pas(User &user) override;

    Status get_user_id_by_log(User &user) override;

    Status del_user(const User &user) override;

    Status make_dialog_request(const User &from_user, const User &to_user) override;

    Status get_user_dialog_requests(const User &user, std::vector<User> &requests) override;

    Status close_dialog_request(const User &from_user, const User &to_user) override;

    // Dialog
    Status make_dialog(Dialog &dialog) override;

    Status change_dialog(const Dialog &new_dialog) override;

    Status get_n_users_dialogs_by_time(
        const User &user,
        std::list<Dialog> &next_dialogs,
        int n = 10,
        int last_dialog_date_time = 2121283574
    ) override {
        for (auto it = dialogs.begin(); it != dialogs.end(); it++) {
            next_dialogs.clear();
            bool fnd = false;
            for (auto ptr = dialog_users[it->m_dialog_id].begin(); ptr != dialog_users[it->m_dialog_id].end(); ptr++){
                if (ptr->m_user_id == user.m_user_id){
                    fnd = true;
                    break;
                }
            }
            if (fnd && it->m_date_time < last_dialog_date_time) {
                for (auto k = next_dialogs.begin(); k != next_dialogs.end();
                     k++) {
                    if (k->m_date_time < it->m_date_time) {
                        next_dialogs.insert(k, *it);
                        break;
                    }
                }
                if (next_dialogs.size() >= n) {
                    next_dialogs.pop_back();
                }
            }
        }
        return Status(true, "Get n dialogs in mock_bd");
    }

    Status del_dialog(const Dialog &dialog) override;

    // Message
    Status make_message(Message &message) override;

    Status change_message(const Message &new_message) override;

    Status get_n_dialogs_messages_by_time(
        const Dialog &dialog,
        std::list<Message> &next_messages,
        int n = 10,
        int last_message_date_time = 2121283574
    ) override {
        for (auto it = messages.begin(); it != messages.end(); it++) {
            next_messages.clear();
            if (it->m_dialog_id == dialog.m_dialog_id &&
                it->m_date_time < last_message_date_time) {
                for (auto k = next_messages.begin(); k != next_messages.end();
                     k++) {
                    if (k->m_date_time < it->m_date_time) {
                        next_messages.insert(k, *it);
                        break;
                    }
                }
                if (next_messages.size() >= n) {
                    next_messages.pop_back();
                }
            }
        }
        return Status(true, "Get n messages in mock_bd");
    }

    Status del_message(const Message &message) override;
};
}  // namespace database_interface

#endif  // DATA_BASE_INTERFACE_HPP