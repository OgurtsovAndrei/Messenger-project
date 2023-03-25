#include <sqlite3.h>
#include <iostream>
#include "include/DataBaseInterface.hpp"
#include "include/User.hpp"

// Start working with bd:
// database_interface::SQL_BDInterface bd =
// database_interface::SQL_BDInterface();

// Open bd:
// bd.open();

// Close bd:
// bd.close();

// Make user:
// database_interface::User usr = database_interface::User(string Name, string
// Surname, string Login, string Password, optional int id); OR you can use
// this: database_interface::User usr = database_interface::User(string Login,
// string Password);

// Add user:
// bd.add_user(usr);

// Get user:
// bd.get_user_log_pas(usr);

// Chang user (class user should be correct, means "id" and others)
// bd.change_user(usr)

int main(int argc, char **argv) {
    //    database_interface::Mock_BDInterface bd =
    //    database_interface::Mock_BDInterface(); std::cout <<
    //    bd.open().m_message << '\n'; database_interface::User usr1 =
    //    database_interface::User("name1", "surname1", "login1",
    //    "password_hash1"); database_interface::User usr2 =
    //    database_interface::User("name2", "surname2", "login2",
    //    "password_hash2"); database_interface::User usr3 =
    //    database_interface::User("name3", "surname3", "login3",
    //    "password_hash3"); database_interface::User usr4 =
    //    database_interface::User("name4", "surname4", "login4",
    //    "password_hash4"); database_interface::User usr5 =
    //    database_interface::User("name5", "surname5", "login5",
    //    "password_hash5"); database_interface::User usr6 =
    //    database_interface::User("name6", "surname6", "login6",
    //    "password_hash6"); std::cout << bd.add_user(usr1).m_message <<
    //    usr1.m_user_id << '\n'; std::cout << bd.add_user(usr2).m_message <<
    //    usr2.m_user_id << '\n'; std::cout << bd.add_user(usr3).m_message <<
    //    usr3.m_user_id << '\n'; std::cout << bd.add_user(usr4).m_message <<
    //    usr4.m_user_id << '\n'; std::cout << bd.add_user(usr5).m_message <<
    //    usr5.m_user_id << '\n'; std::cout << bd.add_user(usr6).m_message <<
    //    usr6.m_user_id << '\n'; database_interface::Dialog dialog123 =
    //    database_interface::Dialog("name123", "encryption123", 123, 0, {usr1,
    //    usr2, usr3}); database_interface::Dialog dialog456 =
    //    database_interface::Dialog("name456", "encryption456", 456, 5, {usr4,
    //    usr5, usr6}); database_interface::Dialog dialog135 =
    //    database_interface::Dialog("name135", "encryption135", 135, 2, {usr1,
    //    usr2, usr3}); std::cout << bd.close().m_message << '\n';
    //    database_interface::SQL_BDInterface bd =
    //        database_interface::SQL_BDInterface();
    //    database_interface::User usr =
    //        database_interface::User("antisuper", "antipuper", "mainy",
    //        "stop", 0);
    //    bd.open();
    //    //    bd.add_user(usr);
    //    //    bd.get_user_log_pas(usr);
    //    //    bd.change_user(usr).m_correct;
    //    bd.close();
    //    std::cout << usr.m_user_id << ' ' << usr.m_name << ' ' <<
    //    usr.m_surname
    //              << ' ' << usr.m_login << ' ' << usr.m_password_hash << '\n';
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
