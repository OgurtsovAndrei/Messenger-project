#ifndef CHAT_HPP
#define CHAT_HPP

#include <string>
#include "Dialog.hpp"
#include "User.hpp"

namespace database_interface {

struct Chat : Dialog {
    std::pair<User, User> m_users;

    Chat(
        int chat_id,
        const std::string &encryption,
        const std::pair<User, User> &users
    )
        : Dialog(chat_id, encryption), m_users(users) {
    }
};

}  // namespace database_interface
#endif  // CHAT_HPP