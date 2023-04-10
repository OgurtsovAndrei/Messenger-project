#include "userUI.h"

void UserUI::set_user_name(const std::string &name) {
    user_name = name;
}

void UserUI::set_user_surname(const std::string &sName) {
    user_name = sName;
}

[[nodiscard]] unsigned int UserUI::get_user_id() const {
    return user_id;
}

[[nodiscard]] std::string UserUI::get_user_name() const {
    return user_name;
}

[[nodiscard]] std::string UserUI::get_user_surname() const {
    return user_surname;
}
