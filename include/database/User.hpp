#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <vector>

namespace database_interface {

struct User {
    int m_user_id = -1;
    std::string m_name = "";
    std::string m_surname = "";
    std::string m_login = "";
    std::string m_password_hash = "";

    static std::vector<User> *m_requests;
    static User *m_edit_user;

    explicit User() = default;

    explicit User(int user_id) : m_user_id(user_id){
    }

    explicit User(
        const std::string &name,
        const std::string &surname,
        const std::string &login,
        const std::string &password_hash,
        int user_id = -1
    )
        : m_user_id(user_id),
          m_name(name),
          m_surname(surname),
          m_login(login),
          m_password_hash(password_hash) {
    }

    explicit User(const std::string &login, const std::string &password_hash)
        : m_login(login), m_password_hash(password_hash) {
    }

    explicit User(int id, const std::string &name, const std::string &surname)
            : m_user_id(id), m_name(name), m_surname(surname) {
    }

    static int callback(void *NotUsed, int argc, char **argv, char **azColName);

    static int request_callback(void *NotUsed, int argc, char **argv, char **azColName);
};

}  // namespace database_interface
#endif  // USER_HPP