#include <sqlite3.h>
#include <iostream>

#include "include/DataBaseInterface.hpp"
#include "include/User.hpp"

int main(int argc, char **argv) {
    std::cout << 1;
    database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();
    database_interface::User usr = database_interface::User("a", "b", "c", "dasc");
    bd.open();
//    bd.add_user(usr);
    bd.get_user("sd", "dsac", usr);
    bd.close();
    //
    //    std::string sql = "CREATE TABLE PERSON("
    //                    "ID INT PRIMARY KEY  NOT NULL, "
    //                    "NAME        TEXT NOT NULL, "
    //                    "SURNAME         TEXT    NOT NULL, "
    //                    "AGE         INT     NOT NULL, "
    //                    "ADDRESS     CHAR(50), "
    //                    "SALARY      REAL );";
    //    int exit = 0;
    //    exit = sqlite3_open("example.db", &DB);
    //    char* messaggeError;
    //    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messaggeError);
    //
    //    if (exit != SQLITE_OK) {
    //        std::cerr << "Error Create Table" << std::endl;
    //        sqlite3_free(messaggeError);
    //    }
    //    else
    //        std::cout << "Table created Successfully" << std::endl;
    //    sqlite3_close(DB);
    return (0);
}
