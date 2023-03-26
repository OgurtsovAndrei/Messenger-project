#ifndef DATA_BASE_INTERFACE_HPP
#define DATA_BASE_INTERFACE_HPP

#include <sqlite3.h>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include "Dialog.hpp"
#include "Message.hpp"
#include "Status.hpp"
#include "User.hpp"

namespace database_interface {

struct BDInterface {
    sqlite3 *m_bd;

    // Work with bd connection
    virtual Status open() = 0;

    virtual Status close() = 0;

    // User
    virtual Status add_user(User &user) = 0;

    virtual Status change_user(const User &new_user) = 0;

    virtual Status get_user_by_log_pas(User &user) = 0;

    virtual Status get_user_id_by_log(User &user) = 0;

    virtual Status del_user(const User &user) = 0;

    virtual Status
    make_dialog_request(const User &from_user, const User &to_user) = 0;

    virtual Status
    close_dialog_request(const User &from_user, const User &to_user) = 0;

    // Dialog
    virtual Status make_dialog(Dialog &dialog) = 0;

    virtual Status change_dialog(const Dialog &new_dialog) = 0;

    virtual Status get_n_users_dialogs_by_time(
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
    // Work with bd connection
    Status open();

    Status close();

    // User
    Status add_user(User &user);

    Status change_user(const User &new_user);

    Status get_user_by_log_pas(User &user);

    Status del_user(const User &user);

    Status make_dialog_request(const User &from_user, const User &to_user);

    Status close_dialog_request(const User &from_user, const User &to_user);

    // Dialog
    Status make_dialog(Dialog &dialog);

    Status change_dialog(const Dialog &new_dialog);

    Status get_n_users_dialogs_by_time(
        const User &user,
        std::list<Dialog> &next_dialogs,
        int n = 10,
        int last_dialog_date_time = 2121283574
    );

    Status del_dialog(const Dialog &dialog);

    // Message
    Status make_message(Message &message);

    Status change_message(const Message &new_message);

    Status get_n_dialogs_messages_by_time(
        const Dialog &dialog,
        std::list<Message> &next_messages,
        int n = 10,
        int last_message_date_time = 2121283574
    );

    Status del_message(const Message &message);
};

struct Mock_BDInterface : BDInterface {
    int last_user_id = 0;
    int last_dialog_id = 0;
    int last_message_id = 0;
    std::map<std::string, User> users;
    std::map<std::string, std::list<User>> requests;
    std::list<Dialog> dialogs;
    std::list<Message> messages;

    // Work with bd connection
    Status open() {
        return Status(true, "Open mock_bd");
    }

    Status close() {
        return Status(true, "Close mock_bd");
    }

    // User
    Status add_user(User &user);

    Status change_user(const User &new_user);

    Status get_user_by_log_pas(User &user);

    Status get_user_id_by_log(User &user);

    Status del_user(const User &user);

    Status make_dialog_request(const User &from_user, const User &to_user);

    Status close_dialog_request(const User &from_user, const User &to_user);

    // Dialog
    Status make_dialog(Dialog &dialog);

    Status change_dialog(const Dialog &new_dialog);

    Status get_n_users_dialogs_by_time(
        const User &user,
        std::list<Dialog> &next_dialogs,
        int n = 10,
        int last_dialog_date_time = 2121283574
    ) {
        for (auto it = dialogs.begin(); it != dialogs.end(); it++) {
            next_dialogs.clear();
            if (it->find(user) && it->m_date_time < last_dialog_date_time) {
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

    Status del_dialog(const Dialog &dialog);

    // Message
    Status make_message(Message &message);

    Status change_message(const Message &new_message);

    Status get_n_dialogs_messages_by_time(
        const Dialog &dialog,
        std::list<Message> &next_messages,
        int n = 10,
        int last_message_date_time = 2121283574
    ) {
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

    Status del_message(const Message &message);
};
}  // namespace database_interface

#endif  // DATA_BASE_INTERFACE_HPP