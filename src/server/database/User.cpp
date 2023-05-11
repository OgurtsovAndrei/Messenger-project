#include "database/User.hpp"
#include <vector>

namespace database_interface {

int User::callback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc < 3){
        return 0;
    }
    m_edit_user->m_user_id = std::stoi(argv[0]);
    m_edit_user->m_name = argv[1];
    m_edit_user->m_surname = argv[2];
    return 0;
}

int User::request_callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i+=3){
        m_requests->push_back(User(std::stoi(argv[i]), argv[i+1], argv[i+2]));
    }
    return 0;
}

int User::get_all_params(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc < 4){
        return 0;
    }
    m_edit_user->m_user_id = std::stoi(argv[0]);
    m_edit_user->m_name = argv[1];
    m_edit_user->m_surname = argv[2];
    m_edit_user->m_password_hash = argv[3];
    return 0;
}


User *User::m_edit_user = nullptr;
std::vector<User> *User::m_requests = nullptr;
}  // namespace database_interface