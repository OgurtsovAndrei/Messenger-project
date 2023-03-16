#include <sqlite3.h>
#include <iostream>

#include "include/DataBaseInterface.hpp"
#include "include/User.hpp"


// Start working with bd:
//database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();

// Open bd:
// bd.open();

// Close bd:
// bd.close();

// Make user:
// database_interface::User usr = database_interface::User(string Name, string Surname, string Login, string Password, optional int id);
// OR you can use this:
// database_interface::User usr = database_interface::User(string Login, string Password);

// Add user:
// bd.add_user(usr);

// Get user:
// bd.get_user_log_pas(usr);



int main(int argc, char **argv) {
    database_interface::SQL_BDInterface bd = database_interface::SQL_BDInterface();
    database_interface::User usr = database_interface::User("antisuper", "antipuper", "mainy", "stop");
    bd.open();
//    bd.add_user(usr);
//    bd.get_user_log_pas(usr);
    bd.close();
    std::cout << usr.m_user_id << ' ' << usr.m_name << ' ' << usr.m_surname << ' ' << usr.m_login << ' ' << usr.m_password_hash << '\n';
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
