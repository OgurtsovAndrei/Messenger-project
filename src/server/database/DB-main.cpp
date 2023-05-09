//#include <sqlite3.h>
//#include <iostream>
//#include <list>
//#include <chrono>
//#include <ctime>
//#include "../../../include/database/DataBaseInterface.hpp"
//#include "../../../include/database/User.hpp"
//
//int main(int argc, char **argv) {
//    database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();
//    bd.open();
//    database_interface::User user;
//    user.m_login = "A";
//    user.m_password_hash = "A-password";
//    std::cout << bd.get_user_by_log_pas(user).correct() << bd.get_user_by_log_pas(user).message() << '\n';
//    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << '|' << user.m_login << '|' << user.m_password_hash << ">\n";
//    bd.close();
//    return (0);
//}