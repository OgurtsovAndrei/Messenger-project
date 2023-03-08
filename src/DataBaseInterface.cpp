#include <sqlite3.h>
#include <iostream>

namespace database_interface {

struct Satus {
    bool correct;
    std::string message;

    Status(bool res, std::string s) : correct(res), message(s) {
    }

    ~Status() = default;
};

struct BDInterface {
    sqlite3 *bd;

    virtual Status open() = 0;

    //
    //  virtual Status add_user(const User& user) = 0;
    //
    //  virtual Status change_user(const User& old_user, const User& new_user) =
    //  0;
    //
    //  virtual User get_user(const User& pre_user) = 0;
    //
    //  virtual Status del_user(const User& user) = 0;
    //
    //
    //
    //  virtual Status make_dialog_request(const User& from_user, const User&
    //  to_user) = 0;
    //
    //  virtual Status close_dilog_request(const User& from_user, const User&
    //  to_user) = 0;
    //
    //
    //
    //  virtual Status make_chat(const User& user1, const User& user2) = 0;
    //
    //  virtual Status change_chat(const Chat& chat) = 0;
    //
    ////  virtual Status get_chat(const User& user1, const User& user2) = 0;
    //
    //  virtual Status del_chat(const User& user1, const User& user2) = 0;
    //
    //
    //
    //  virtual Status make_group(const User& user) = 0;
    //
    //  virtual Status change_chat(const Group& old_group, const Group&
    //  new_group) = 0;
    //
    // //  virtual Status get_chat(const User& user1, const User& user2) = 0;
    //
    //  virtual Status del_group(const Group& group) = 0;
    //
    //
    //
    virtual Status close() = 0;
};

struct SQL_BDInterface : BDInterface {
    Status open() {
        int exit = 0;
        exit = sqlite3_open("ServerDataBase.db", &bd);
        return Status(exit != SQLITE_OK, "Problem in database open\n");
    }

    void close() {
        int exit = 0;
        exit = sqlite3_close(bd);
        // Почитать возврат значения при закрытии бд.
        return Status(exit != SQLITE_OK, "Problem in database close\n");
    }
};

}  // namespace database_interface