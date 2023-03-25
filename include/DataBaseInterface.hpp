#ifndef DATA_BASE_INTERFACE_HPP
#define DATA_BASE_INTERFACE_HPP

#include <sqlite3.h>
#include <iostream>
#include <map>
#include <set>
#include <list>
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

        virtual Status get_user_log_pas(User &user) = 0;

        virtual Status del_user(const User &user) = 0;
        //    virtual Status
        //    make_dialog_request(const User &from_user, const User &to_user) = 0;
        //    virtual Status
        //    close_dialog_request(const User &from_user, const User &to_user) = 0;
        //
        //    // Dialog
        //    virtual Status make_dialog(const Dialog &dialog) = 0;
        //    virtual Status change_dialog(const Dialog &new_dialog) = 0;
        //    virtual Status get_n_users_dialogs_by_time(
        //        const User &user,
        //        int n = 10,
        //        int last_dialog_date_time = 2121283574
        //    ) = 0;
        //    virtual Status del_dialog(const Dialog &dialog) = 0;
        //
        //    // Message
        //    virtual Status make_message(const Message &message) = 0;
        //    virtual Status change_message(const Message &new_message) = 0;
        //    virtual Status get_n_dialogs_messages_by_time(
        //        const Dialog &dialog,
        //        std::vector<Dialog> next_dialogs,
        //        int n = 10,
        //        int last_message_date_time = 2121283574
        //    ) = 0;
        //    virtual Status del_message(const Message &Message) = 0;
    };

//    struct SQL_BDInterface : BDInterface {
//        // Work with bd connection
//        Status open();
//
//        Status close();
//
//        // User
//        Status add_user(User &user);
//
//        Status change_user(const User &new_user);
//
//        Status get_user_log_pas(User &user);
//
//        Status del_user(const User &user);
//        //    Status make_dialog_request(const User &from_user, const User
//        //    &to_user); Status close_dilog_request(const User &from_user, const
//        //    User &to_user);
//        //
//        //    // Dialog
//        //    virtual Status make_dialog(const Dialog &dialog) = 0;
//        //    virtual Status change_dialog(const Dialog &new_dialog) = 0;
//        //    virtual Status get_n_users_dialogs_by_time(
//        //        const User &user,
//        //        std::vector<Dialog> next_dialogs,
//        //        int n = 10,
//        //        int last_dialog_date_time = 2121283574
//        //    ) = 0;
//        //    virtual Status del_dialog(const Dialog &dialog) = 0;
//        //
//        //    // Message
//        //    virtual Status make_message(const Message &message) = 0;
//        //    virtual Status change_message(const Message &new_message) = 0;
//        //    virtual Status get_n_dialogs_messages_by_time(
//        //        const Dialog &dialog,
//        //        int n = 10,
//        //        int last_message_date_time = 2121283574
//        //    ) = 0;
//        //    virtual Status del_message(const Message &Message) = 0;
//    };

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
        Status add_user(User &user) {
            if (user.m_user_id == -1) {
                user.m_user_id = last_user_id++;
            }
            users[user.m_login] = user;
            return Status(true, "Add user in mock_bd");
        }

        Status change_user(const User &new_user) {
            users[new_user.m_login] = new_user;
            return Status(true, "Change user in mock_bd");
        }

        Status get_user_log_pas(User &user) {
            if (users[user.m_login].m_password_hash == user.m_password_hash) {
                user = users[user.m_login];
                return Status(true, "Get user in mock_bd");
            }
            return Status(false, "Can not get user in mock_bd");
        }

        Status del_user(const User &user) {
            users.erase(users.find(user.m_login));
            return Status(true, "Delete user in mock_bd");
        }

        Status make_dialog_request(const User &from_user, const User &to_user) {
            for (auto it = requests[to_user.m_login].begin(); it != requests[to_user.m_login].end(); it++){
                if (it->m_user_id == from_user.m_user_id){
                    return Status(true, "Make dialog request in mock_bd");
                }
            }
            requests[to_user.m_login].insert(requests[to_user.m_login].end(), from_user);
            return Status(true, "Make dialog request in mock_bd");
        }

        Status close_dialog_request(const User &from_user, const User &to_user) {
            for (auto it = requests[to_user.m_login].begin(); it != requests[to_user.m_login].end(); it++){
                if (it->m_user_id == from_user.m_user_id){
                    requests[to_user.m_login].erase(it);
                    return Status(true, "Close dialog request in mock_bd");
                }
            }
            return Status(false, "Can not find request in mock_bd");
        }

        // Dialog
        Status make_dialog(Dialog &dialog) {
            dialog.m_dialog_id = last_dialog_id++;
            dialogs.push_back(dialog);
            return Status(true, "Make dialog in mock_bd");
        }

        Status change_dialog(const Dialog &new_dialog) {
            for (auto it = dialogs.begin(); it != dialogs.end(); it++) {
                if (it->m_dialog_id == new_dialog.m_dialog_id) {
                    *it = new_dialog;
                    return Status(true, "Change dialog in mock_bd");
                }
            }
            return Status(false, "Can not change dialog in mock_bd");
        }

        Status get_n_users_dialogs_by_time(
                const User &user,
                std::list <Dialog> &next_dialogs,
                int n = 10,
                int last_dialog_date_time = 2121283574
        ) {
            for (auto it = dialogs.begin(); it != dialogs.end(); it++) {
                next_dialogs.clear();
                if (it->find(user) &&
                    it->m_date_time < last_dialog_date_time) {
                    for (auto k = next_dialogs.begin(); k != next_dialogs.end(); k++) {
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

        Status del_dialog(const Dialog &dialog) {
            for (auto it = dialogs.begin(); it != dialogs.end(); it++) {
                if (it->m_dialog_id == dialog.m_dialog_id) {
                    dialogs.erase(it);
                    return Status(true, "Delete dialog in mock_bd");
                }
            }
            return Status(false, "Can not delete dialog in mock_bd");
        }

        // Message
        Status make_message(Message &message) {
            message.m_message_id = last_message_id++;
            messages.push_back(message);
            return Status(true, "Make message in mock_bd");
        }

        Status change_message(const Message &new_message) {
            for (auto it = messages.begin(); it != messages.end(); it++) {
                if (it->m_message_id == new_message.m_message_id) {
                    *it = new_message;
                    return Status(true, "Change message in mock_bd");
                }
            }
            return Status(false, "Can not change message in mock_bd");
        }

        Status get_n_dialogs_messages_by_time(
                const Dialog &dialog,
                std::list <Message> &next_messages,
                int n = 10,
                int last_message_date_time = 2121283574
        ) {
            for (auto it = messages.begin(); it != messages.end(); it++) {
                next_messages.clear();
                if (it->m_dialog_id == dialog.m_dialog_id &&
                    it->m_date_time < last_message_date_time) {
                    for (auto k = next_messages.begin(); k != next_messages.end(); k++) {
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

        Status del_message(const Message &message) {
            for (auto it = messages.begin(); it != messages.end(); it++) {
                if (it->m_message_id == message.m_dialog_id) {
                    messages.erase(it);
                    return Status(true, "Delete message in mock_bd");
                }
            }
            return Status(false, "Can not delete message in mock_bd");
        }
    };
}  // namespace database_interface

#endif  // DATA_BASE_INTERFACE_HPP