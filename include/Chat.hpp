#ifndef CHAT_HPP
#define CHAT_HPP

#include <string>
#include "User.hpp"

namespace database_interface {

struct Chat {
    int m_chat_id;
    int m_date_time;
    std::string m_encryption;
    std::vector<User> m_users;

    Chat(
        int chat_id,
        int date_time,
        const std::string &encryption,
        const std::vector<User> &users
    )
        : m_chat_id(chat_id), m_date_time(date_time), m_encryption(encryption), m_users(users) {
    }
};

}  // namespace database_interface
#endif  // CHAT_HPP