#ifndef CHAT_HPP
#define CHAT_HPP

#include <string>
#include <vector>
#include "User.hpp"

namespace database_interface {

struct Chat {
    int m_chat_id;
    std::string m_encryption;
    std::vector<User> m_users;

    Chat(
        int chat_id,
        const std::string &encryption,
        const std::vector<User> &users
    )
        : m_chat_id(chat_id), m_encryption(encryption), m_users(users) {
    }
};

}  // namespace database_interface
#endif  // CHAT_HPP