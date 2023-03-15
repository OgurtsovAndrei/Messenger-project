#ifndef DATA_BASE_INTERFACE_HPP
#define DATA_BASE_INTERFACE_HPP

#include <sqlite3.h>
#include <iostream>
#include "Chat.hpp"
#include "Group.hpp"
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
//    virtual Status change_user(const User &new_user) = 0;
    virtual Status
    get_user(std::string login, std::string password_hash, User &user) = 0;
//    virtual Status del_user(const User &user) = 0;
//    virtual Status
//    make_dialog_request(const User &from_user, const User &to_user) = 0;
//    virtual Status
//    close_dilog_request(const User &from_user, const User &to_user) = 0;
//
//    // Chat
//    virtual Status make_chat(const Chat &chat) = 0;
//    virtual Status change_chat(const Chat &new_chat) = 0;
//    virtual Status
//    get_chat(const User &user1, const User &user2, Chat &chat) = 0;
//    virtual Status del_chat(const Chat &chat) = 0;
//
//    // Group
//    virtual Status make_group(const Group &group) = 0;
//    virtual Status change_group(const Group &new_group) = 0;
//    virtual Status get_n_users_groups_by_time(
//        const User &user,
//        int n = 10,
//        int last_group_id = -1
//    ) = 0;
//    virtual Status del_group(const Group &group) = 0;
//
//    // Message
//    virtual Status make_message(const Message &message) = 0;
//    virtual Status change_message(const Group &new_message) = 0;
//    virtual Status get_n_chats_messages_by_time(
//        const Chat &chat,
//        int n = 10,
//        int last_message_id = -1
//    ) = 0;
//    virtual Status del_message(const Message &Message) = 0;
};

struct SQL_BDInterface : BDInterface {
    // Work with bd connection
    Status open();
    Status close();

    // User
    Status add_user(User &user);
//    Status change_user(const User &new_user);
    Status get_user(std::string login, std::string password_hash, User &user);
//    Status del_user(const User &user);
//    Status make_dialog_request(const User &from_user, const User &to_user);
//    Status close_dilog_request(const User &from_user, const User &to_user);
//
//    // Chat
//    Status make_chat(const Chat &chat);
//    Status change_chat(const Chat &new_chat);
//    Status get_chat(const User &user1, const User &user2, Chat &chat);
//    Status del_chat(const Chat &chat);
//
//    // Group
//    Status make_group(const Group &group);
//    Status change_group(const Group &new_group);
//    Status get_n_users_groups_by_time(
//        const User &user,
//        int n = 10,
//        int last_group_id = -1
//    );
//    Status del_group(const Group &group);
//
//    // Message
//    Status make_message(const Message &message);
//    Status change_message(const Group &new_message);
//    Status get_n_chats_messages_by_time(
//        const Chat &chat,
//        int n = 10,
//        int last_message_id = -1
//    );
//    Status del_message(const Message &Message);
};
}  // namespace database_interface

#endif  // DATA_BASE_INTERFACE_HPP