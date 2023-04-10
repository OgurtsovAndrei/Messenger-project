#include "userUI.h"

void UserUi::set_user_id(const int &id) {
    user_id = id;
}

void UserUi::set_user_name(const std::string &name) {
    user_name = name;
}

void UserUi::set_user_surname(const std::string &sName) {
    user_name = sName;
}

[[nodiscard]] int UserUi::get_user_id() const {
    return user_id;
}

[[nodiscard]] std::string UserUi::get_user_name() const {
    return user_name;
}

[[nodiscard]] std::string UserUi::get_user_surname() const {
    return user_surname;
}

UserUi user;