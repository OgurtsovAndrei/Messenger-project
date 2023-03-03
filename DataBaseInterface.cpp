#include <iostream>
#include <sqlite3.h>


struct ChatManager {

  virtual Status open() = 0;



  virtual Status add_user(const User& user) = 0;

  virtual Status change_user(const User& old_user, const User& new_user) = 0;

  virtual User get_user(const User& pre_user) = 0;

  virtual Status del_user(const User& user) = 0;



  virtual Status make_dialog_request(const User& from_user, const User& to_user) = 0;

  virtual Status close_dilog_request(const User& from_user, const User& to_user) = 0;



  virtual Status make_chat(const User& user1, const User& user2) = 0;

  virtual Status change_chat(const Chat& chat) = 0;  

//  virtual Status get_chat(const User& user1, const User& user2) = 0;

  virtual Status del_chat(const User& user1, const User& user2) = 0;



  virtual Status make_group(const User& user) = 0;

  virtual Status change_chat(const Group& old_group, const Group& new_group) = 0;  

 //  virtual Status get_chat(const User& user1, const User& user2) = 0;

  virtual Status del_group(const Group& group) = 0;

  

  virtual Status close() = 0;
};

class SqlChatManager : public ChatManager {
 public:
  Status AddUser(const User& user) override {
     <Build SQL query ….>
  }
 private:
  DbConnection connection_;
};

class KeyValueChatManager : public ChatManager {
  ….
};

class MockChatManager : public ChatManager {
  public:
   Status AddUser(const User& user) override {
      users_[user.name] = user;
      return OK;
    }
  private:
    std::unordered_map<string, User> users_;
};



int main(int argc, char** argv)
{
    sqlite3* DB;
    std::string sql = "CREATE TABLE PERSON("
                    "ID INT PRIMARY KEY  NOT NULL, "
                    "NAME        TEXT NOT NULL, "
                    "SURNAME         TEXT    NOT NULL, "
                    "AGE         INT     NOT NULL, "
                    "ADDRESS     CHAR(50), "
                    "SALARY      REAL );";
    int exit = 0;
    exit = sqlite3_open("example.db", &DB);
    char* messaggeError;
    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messaggeError);

    if (exit != SQLITE_OK) {
        std::cerr << "Error Create Table" << std::endl;
        sqlite3_free(messaggeError);
    }
    else
        std::cout << "Table created Successfully" << std::endl;
    sqlite3_close(DB);
    return (0);
}
