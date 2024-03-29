#include <sqlite3.h>
#include <iostream>
#include "database/DataBaseInterface.hpp"

namespace database_interface {

// User
Status Mock_BDInterface::make_user(User &user) {
    if (user.m_user_id == -1) {
        user.m_user_id = last_user_id++;
    }
    users[user.m_login] = user;
    return Status(true, "Make user in mock_bd");
}

Status Mock_BDInterface::change_user(const User &new_user) {
    users[new_user.m_login] = new_user;
    return Status(true, "Change user in mock_bd");
}

Status Mock_BDInterface::get_user_by_log_pas(User &user) {
    if (users[user.m_login].m_password_hash == user.m_password_hash) {
        user = users[user.m_login];
        return Status(true, "Get user in mock_bd");
    }
    return Status(false, "Can not get user in mock_bd");
}

Status Mock_BDInterface::get_user_id_by_log(User &user) {
    if (users[user.m_login].m_user_id != -1) {
        user.m_user_id = users[user.m_login].m_user_id;
        return Status(true, "Get user id in mock_bd");
    }
    return Status(false, "Can not get user id in mock_bd");
}

Status Mock_BDInterface::del_user(const User &user) {
    users.erase(users.find(user.m_login));
    return Status(true, "Delete user in mock_bd");
}

Status Mock_BDInterface::get_encryption_name_by_id(
    int encryption_id,
    std::string &encryption_name
) {
    if (encryption_id == 1) {
        encryption_name = "RSA";
    } else if (encryption_id == 2) {
        encryption_name = "DSA";
    } else {
        encryption_name = "DH";
    }
    return Status(true, "find encryption");
}

// Dialog
Status Mock_BDInterface::make_dialog(Dialog &dialog) {
    dialog.m_dialog_id = last_dialog_id++;
    dialogs.push_back(dialog);
    return Status(true, "Make dialog in mock_bd");
}

Status Mock_BDInterface::change_dialog(const Dialog &new_dialog) {
    for (auto it = dialogs.begin(); it != dialogs.end(); it++) {
        if (it->m_dialog_id == new_dialog.m_dialog_id) {
            *it = new_dialog;
            return Status(true, "Change dialog in mock_bd");
        }
    }
    return Status(false, "Can not change dialog in mock_bd");
}

Status Mock_BDInterface::del_dialog(const Dialog &dialog) {
    for (auto it = dialogs.begin(); it != dialogs.end(); it++) {
        if (it->m_dialog_id == dialog.m_dialog_id) {
            dialogs.erase(it);
            return Status(true, "Delete dialog in mock_bd");
        }
    }
    return Status(false, "Can not delete dialog in mock_bd");
}

// Message
Status Mock_BDInterface::make_message(Message &message) {
    message.m_message_id = last_message_id++;
    messages.push_back(message);
    return Status(true, "Make message in mock_bd");
}

Status Mock_BDInterface::change_message(const Message &new_message) {
    for (auto it = messages.begin(); it != messages.end(); it++) {
        if (it->m_message_id == new_message.m_message_id) {
            *it = new_message;
            return Status(true, "Change message in mock_bd");
        }
    }
    return Status(false, "Can not change message in mock_bd");
}

Status Mock_BDInterface::del_message(const Message &message) {
    for (auto it = messages.begin(); it != messages.end(); it++) {
        if (it->m_message_id == message.m_dialog_id) {
            messages.erase(it);
            return Status(true, "Delete message in mock_bd");
        }
    }
    return Status(false, "Can not delete message in mock_bd");
}

}  // namespace database_interface