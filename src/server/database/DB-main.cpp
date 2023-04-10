//#include <sqlite3.h>
//#include <iostream>
//#include <list>
//#include "../../../include/database/DataBaseInterface.hpp"
//#include "../../../include/database/User.hpp"
//
//int main(int argc, char **argv) {
//    database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();
//    bd.open();
//    std::list<database_interface::Dialog> next_dialogs;
//    bd.get_n_users_dialogs_by_time(database_interface::User(2), next_dialogs);
//    std::cout << next_dialogs.size();
//    bd.close();
//    return (0);
//}