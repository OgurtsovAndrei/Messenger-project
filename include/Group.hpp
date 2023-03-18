#ifndef GROUP_HPP
#define GROUP_HPP

#include <set>
#include <string>
#include "Chat.hpp"

namespace database_interface {

struct Group : Dialog {
    std::string m_name;
    std::string m_owner;

    std::set<User> m_users

    Group(
        int group_id,
        const std::string &name,
        const std::string &owner,
        const std::string &encryption,
        const std::set<User> &users,
    )
        : Dialog(group_id, encryption),
          m_name(name),
          m_owner(owner),
          m_users(users) {
    }
};

}  // namespace database_interface
#endif  // GROUP_HPP