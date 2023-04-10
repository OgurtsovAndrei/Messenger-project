#ifndef USERUI_H
#define USERUI_H

#include <string>

class UserUi {
public:
    UserUi() = default;

    void set_user_id(const int &id);

    void set_user_name(const std::string &name);

    void set_user_surname(const std::string &sName);

    [[nodiscard]] int get_user_id() const;

    [[nodiscard]] std::string get_user_name() const;

    [[nodiscard]] std::string get_user_surname() const;

private:
    int user_id = -1;
    std::string user_name;
    std::string user_surname;
};

//extern UserUi user;

#endif // USERUI_H
