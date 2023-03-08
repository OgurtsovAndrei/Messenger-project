#ifndef CHAT_HPP
#define CHAT_HPP

#include <string>
#include "User.hpp"

namespace database_interface {

struct Chat {
  int m_id;
  std::string m_encryption;
  std::vector<User> m_users;

  Status(int id, const std::string &encryption, const std::vector<User> &users) : m_id(id), m_encryption(encryption), m_users(users) {}
};

} // namespace database_interface
#endif // CHAT_HPP