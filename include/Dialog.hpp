#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <string>
#include <vector>
#include <list>
#include "User.hpp"

namespace database_interface {

struct Dialog {
    int m_dialog_id;
    std::string m_name;
    std::string m_encryption;
    int m_date_time;
    int m_owner_id;
    bool m_is_group;
    std::vector<User> m_users;

    static std::list<Dialog> *m_dialog_list;

    Dialog(
        const std::string &name,
        const std::string &encryption,
        int date_time,
        int owner,
        bool is_group,
        const std::vector<User> &users,
        int dialog_id = -1
    )
        : m_name(name),
          m_encryption(encryption),
          m_date_time(date_time),
          m_owner_id(owner),
          m_is_group(is_group),
          m_users(users),
          m_dialog_id(dialog_id) {
    }

    Dialog(
            int dialog_id,
            const std::string &name,
            const std::string &encryption,
            int date_time,
            int owner,
            bool is_group) :
            m_dialog_id(dialog_id),
            m_name(name),
            m_encryption(encryption),
            m_date_time(date_time),
            m_owner_id(owner),
            m_is_group(is_group){
    }

    bool find(const User &user) {
        for (int i = 0; i < m_users.size(); i++) {
            if (m_users[i].m_login == user.m_login) {
                return true;
            }
        }
        return false;
    }

    static int callback(void *NotUsed, int argc, char **argv, char **azColName);
};

}  // namespace database_interface
#endif  // DIALOG_HPP