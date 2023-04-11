#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <utility>
#include <vector>

namespace database_interface {

struct User {
    int m_user_id = -1;
    std::string m_name;
    std::string m_surname;
    std::string m_login;
    std::string m_password_hash;

    static std::vector<User> *m_requests;
    static User *m_edit_user;

    explicit User() = default;

    explicit User(int user_id) : m_user_id(user_id){
    }

    explicit User(
        std::string name,
        std::string surname,
        std::string login,
        std::string password_hash,
        int user_id = -1
    )
        : m_user_id(user_id),
          m_name(std::move(name)),
          m_surname(std::move(surname)),
          m_login(std::move(login)),
          m_password_hash(std::move(password_hash)) {
    }

    explicit User(std::string login, std::string password_hash)
        : m_login(std::move(login)), m_password_hash(std::move(password_hash)) {
    }
    explicit User(std::string login)
        : m_login(std::move(login)) {
    }

    explicit User(int id, std::string name, std::string surname)
            : m_user_id(id), m_name(std::move(name)), m_surname(std::move(surname)) {
    }

    static int callback(void *NotUsed, int argc, char **argv, char **azColName);

    static int request_callback(void *NotUsed, int argc, char **argv, char **azColName);
};

}  // namespace database_interface
#endif  // USER_HPP