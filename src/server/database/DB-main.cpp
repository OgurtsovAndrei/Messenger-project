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
//    user.m_login = "A-loginы";
//    bd.get_user_id_by_log(user);
//    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << ">\n";
//    user.m_login = "A-logins";
//    bd.get_user_id_by_log(user);
//    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << ">\n";
//    user.m_login = "A-login";
//    bd.get_user_id_by_log(user);
//    std::cout << '<' << user.m_user_id << '|' << user.m_name << '|' << user.m_surname << ">\n";
//    database_interface::Message mes =  database_interface::Message("ads", "", 6, 2);
//    std::cout << bd.make_message(mes).correct();
//    bd.close();
//    return (0);
//}