#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <string>
#include <vector>
#include "User.hpp"

namespace database_interface {

struct Dialog {
    int m_dialog_id;
    std::string m_name;
    std::string m_encryption;
    int m_date_time;
    int m_owner_id;
    std::vector<User> m_users;

    Dialog(
        const std::string &name,
        const std::string &encryption,
        int date_time,
        int owner,
        const std::vector<User> &users,
        int dialog_id = -1
    )
        : m_name(name),
          m_encryption(encryption),
          m_date_time(date_time),
          m_owner_id(owner),
          m_users(users),
          m_dialog_id(dialog_id) {
    }

    bool find(const User &user) {
        for (int i = 0; i < m_users.size(); i++) {
            if (m_users[i].m_login == user.m_login) {
                return true;
            }
        }
        return false;
    }
};

}  // namespace database_interface
#endif  // DIALOG_HPP