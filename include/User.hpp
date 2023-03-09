#ifndef USER_HPP
#define USER_HPP

#include <string>

namespace database_interface {

struct User {
    int m_user_id;
    std::string m_name;
    std::string m_surname;
    std::string m_login;
    std::string m_password_hash;

    Status(
        int user_id = -1,
        const std::string &name,
        const std::string &surname,
        const std::string &login,
        const std::string &password_hash
    )
        : m_user_id(user_id),
          m_name(name),
          m_surname(surname),
          m_login(login),
          m_password_hash(password_hash) {
    }
};

}  // namespace database_interface
#endif  // USER_HPP