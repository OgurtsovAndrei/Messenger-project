#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <string>
#include <utility>
#include "database/User.hpp"

struct ClientInfo {
    std::string cl_name;
    std::string cl_surname;
    std::string cl_login;
    unsigned int cl_id{};
    int cl_encryption_id;

    ClientInfo() = default;

    explicit ClientInfo(database_interface::User user) :
          cl_name(std::move(user.m_name)),
          cl_surname(std::move(user.m_surname)),
          cl_login(std::move(user.m_login)),
          cl_id(user.m_user_id),
          cl_encryption_id(user.m_encryption){}
};

#endif // CLIENTINFO_H
