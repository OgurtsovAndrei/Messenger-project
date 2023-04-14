#include <sqlite3.h>
#include <iostream>
#include <list>
#include "../../../include/database/DataBaseInterface.hpp"
#include "../../../include/database/User.hpp"

int main(int argc, char **argv) {
    database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();
    bd.open();
    database_interface::User user;
    user.m_login = "A-loginы";
    bd.get_user_id_by_log(user);
    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << ">\n";
    user.m_login = "A-logins";
    bd.get_user_id_by_log(user);
    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << ">\n";
    user.m_login = "A-login";
    bd.get_user_id_by_log(user);
    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << ">\n";
    std::cout << bd.del_message(database_interface::Message(5)).correct();
    std::cout << bd.del_user(database_interface::User(4)).correct();
    bd.close();
    return (0);
}