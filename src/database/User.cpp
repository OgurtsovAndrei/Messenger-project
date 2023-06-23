#include "database/User.hpp"
#include <vector>

namespace database_interface {

int User::get_login(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc < 1) {
        return 1;
    }
    m_edit_user->m_login = argv[0];
    return 0;
}

int User::callback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc < 3){
        return 1;
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
    if (argc < 5){
        return 1;
    }
    m_edit_user->m_user_id = std::stoi(argv[0]);
    m_edit_user->m_name = argv[1];
    m_edit_user->m_surname = argv[2];
    m_edit_user->m_password_hash = argv[3];
    m_edit_user->m_encryption = std::stoi(argv[4]);
    return 0;
}

int User::callback_for_encryption_name(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc != 1){
        return 1;
    }
    *m_encryption_name = argv[0];
    return 0;
}

int User::callback_for_all_encryption_names(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i< argc; i+=2){
        m_encryption_pair_id_name->push_back({std::stoi(argv[i]), argv[i+1]});
    }
    return 0;
}

User *User::m_edit_user = nullptr;
std::vector<User> *User::m_requests = nullptr;
std::string *User::m_encryption_name = nullptr;
std::vector<std::pair<int, std::string>> *User::m_encryption_pair_id_name = nullptr;
}  // namespace database_interface