#ifndef DATA_BASE_INTERFACE_HPP
#define DATA_BASE_INTERFACE_HPP

#include <sqlite3.h>
#include <iostream>
#include <map>
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
    virtual Status change_user(const User &new_user) = 0;
    virtual Status get_user_log_pas(User &user) = 0;
    virtual Status del_user(const User &user) = 0;
    //    virtual Status
    //    make_dialog_request(const User &from_user, const User &to_user) = 0;
    //    virtual Status
    //    close_dilog_request(const User &from_user, const User &to_user) = 0;
    //
    //    // Chat
    //    virtual Status make_chat(const Chat &chat) = 0;
    //    virtual Status change_chat(const Chat &new_chat) = 0;
    //    virtual Status del_chat(const Chat &chat) = 0;
    //
    //    // Group
    //    virtual Status make_group(const Group &group) = 0;
    //    virtual Status change_group(const Group &new_group) = 0;
    //    virtual Status get_n_users_groups_by_time(
    //        const User &user,
    //        int n = 10,
    //        int last_chat_id = -1
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
    Status change_user(const User &new_user);
    Status get_user_log_pas(User &user);
    Status del_user(const User &user);
    //    Status make_dialog_request(const User &from_user, const User
    //    &to_user); Status close_dilog_request(const User &from_user, const
    //    User &to_user);
    //
    //    // Chat
    //    Status make_chat(const Chat &chat);
    //    Status change_chat(const Chat &new_chat);
    //    Status del_chat(const Chat &chat);
    //
    //    // Group
    //    Status make_group(const Group &group);
    //    Status change_group(const Group &new_group);
    //    Status get_n_users_groups_by_time(
    //        const User &user,
    //        int n = 10,
    //        int last_chat_id = -1
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

struct Mock_BDInterface : BDInterface {
    std::map<std::string, User> users;
    std::map<User, std::set<User>> requests;
    std::map<int, Chat> chats;
    std::map<int, Group> groups;
    std::map<int, Message> messages;

    // Work with bd connection
    Status open() {
        return Status(true, "Open mock_bd");
    }

    Status close() {
        return Status(true, "Close mock_bd");
    }

    // User
    Status add_user(User &user) {
        users[user.m_name] = user;
        return Status(true, "Add user in mock_bd");
    }

    Status change_user(const User &new_user) {
        users[user.m_name] = new_user;
        return Status(true, "Change user in mock_bd");
    }

    Status get_user_log_pas(User &user) {
        if (user[user.m_name].m_password_hash == user.m_password_hash) {
            user = user[user.m_name];
            return Status(true, "Get user in mock_bd");
        }
        return Status(false, "Can not change user in mock_bd");
    }

    Status del_user(const User &user) {
        users.erase(users.find(user.name));
        return Status(true, "Delete user in mock_bd");
    }

    Status make_dialog_request(const User &from_user, const User &to_user) {
        requests[from_user.m_name].insert(to_user);
        return Status(true, "Make dialog request in mock_bd");
    }

    Status close_dilog_request(const User &from_user, const User &to_user) {
        requests[from_user.m_name].erase(requests[from_user.m_name].find(to_user
        ));
        return Status(true, "Close dialog request in mock_bd");
    }

    // Chat
    Status make_chat(const Chat &chat) {
        chats[chat.m_chat_id] = chat;
        return Status(true, "Make chat in mock_bd");
    }

    Status change_chat(const Chat &new_chat) {
        chats[new_chat.m_chat_id] = new_chat;
        return Status(true, "Change chat in mock_bd");
    }

    Status del_chat(const Chat &chat) {
        chats.erase(chats.find(chat.m_chat_id));
        return Status(true, "Delete chat in mock_bd");
    }

    // Group
    Status make_group(const Group &group){
        groups[group.m_group_id] = group;
        return Status(true, "Make group in mock_bd");
    }
    Status change_group(const Group &new_group){
        groups[new_group.m_group_id] = new_group;
        return Status(true, "Change group in mock_bd");
    }

    Status del_group(const Group &group){
        chats.erase(chats.find(group.m_chat_id));
        groups.erase(groups.find(group.m_group_id));
        return Status(true, "Delete group in mock_bd");
    }


    Status get_n_users_groups_by_time(
        const User &user,
        int n = 10,
        int last_date_time = -1
    ){

    }

    // Message
    Status make_message(const Message &message);
    Status change_message(const Group &new_message);
    Status get_n_chats_messages_by_time(
        const Chat &chat,
        int n = 10,
        int last_message_id = -1
    );
    Status del_message(const Message &Message);
};
}  // namespace database_interface

#endif  // DATA_BASE_INTERFACE_HPP