//#include <sqlite3.h>
//#include <iostream>
//#include <list>
//#include "../../../include/database/DataBaseInterface.hpp"
//#include "../../../include/database/User.hpp"
//
//int main(int argc, char **argv) {
//    database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();
//    bd.open();
//    std::list<database_interface::Message> next_messages;
//    bd.get_n_dialogs_messages_by_time(database_interface::Dialog(2), next_messages);
//    std::cout << next_messages.size();
//    bd.close();
//    return (0);
//}