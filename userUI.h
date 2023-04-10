#ifndef USER_H
#define USER_H


#include <string>

class UserUI {
public:
    UserUI() = default;

    void set_user_name(const std::string &name);

    void set_user_surname(const std::string &sName);

    [[nodiscard]] unsigned int get_user_id() const;

    [[nodiscard]] std::string get_user_name() const;

    [[nodiscard]] std::string get_user_surname() const;

private:
    unsigned int user_id = 0;
    std::string user_name;
    std::string user_surname;
};

#endif // USER_H
