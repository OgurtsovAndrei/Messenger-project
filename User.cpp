#include "include/User.hpp"
#include <iostream>
#include <string>

namespace database_interface {

int User::callback(void *NotUsed, int argc, char **argv, char **azColName) {
    std::string id = argv[0];
    m_edit_user->m_user_id = std::stoi(id);
    m_edit_user->m_name = argv[1];
    m_edit_user->m_surname = argv[2];
    return 0;
}

User *User::m_edit_user = nullptr;
}  // namespace database_interface