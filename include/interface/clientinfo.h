#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <string>

struct ClientInfo {
    std::string cl_name;
    std::string cl_surname;
    std::string cl_login;
    unsigned int cl_id;

    ClientInfo() = default;

    ClientInfo(std::string name, std::string surname, std::string login, unsigned int id) :
        cl_name(name),
        cl_surname(surname),
        cl_login(login),
        cl_id(id) {}
};

#endif // CLIENTINFO_H
