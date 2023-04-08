#ifndef DIALOG_HPP
#define DIALOG_HPP

#include <string>
#include <utility>
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

    explicit Dialog(
        std::string name,
        std::string encryption,
        int date_time,
        int owner,
        bool is_group,
        const std::vector<User> &users,
        int dialog_id = -1
    )
        : m_name(std::move(name)),
          m_encryption(std::move(encryption)),
          m_date_time(date_time),
          m_owner_id(owner),
          m_is_group(is_group),
          m_users(users),
          m_dialog_id(dialog_id) {
    }

    explicit Dialog(
            int dialog_id,
            std::string name,
            std::string encryption,
            int date_time,
            int owner,
            bool is_group) :
            m_dialog_id(dialog_id),
            m_name(std::move(name)),
            m_encryption(std::move(encryption)),
            m_date_time(date_time),
            m_owner_id(owner),
            m_is_group(is_group){
    }

    [[nodiscard]] bool find(const User &user) {
        // NOLINTNEXTLINE
        for (auto & m_user : m_users) {
            if (m_user.m_login == user.m_login) {
                return true;
            }
        }
        return false;
    }

    static int callback(void *NotUsed, int argc, char **argv, char **azColName);
};

}  // namespace database_interface
#endif  // DIALOG_HPP