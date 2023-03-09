#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <vector>
#include "Chat.hpp"

namespace database_interface {

struct Group : Chat {
    int m_group_id;
    std::string m_name;
    std::string m_owner;
    int m_chat_id;

    Group(
        int group_id,
        const std::string &name,
        const std::string &owner,
        const std::string &encryption,
        const std::vector<User> &users,
        int chat_id
    )
        : Chat(chat_id, encryption, users),
          m_group_id(group_id),
          m_name(name),
          m_owner(owner) {
    }
};

}  // namespace database_interface
#endif  // GROUP_HPP