#include "database/Dialog.hpp"
#include <iostream>

namespace database_interface {

int Dialog::callback_get_dialog_users(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++){
        m_users->push_back(User(std::stoi(argv[i])));
    }
    return 0;
}

int Dialog::callback_get_dialogs(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i+=6){
        m_dialogs->push_back(Dialog(std::stoi(argv[i]),
                                        argv[i+1],
                                        argv[i+2],
                                        std::stoi(argv[i+3]),
                                        std::stoi(argv[i+4]),
                                        std::stoi(argv[i+5])));
    }
    return 0;
}

int Dialog::callback_get_one_dialog(void *NotUsed, int argc, char **argv, char **azColName) {
    *m_edit_dialog = Dialog(std::stoi(argv[0]), argv[1],argv[2],std::stoi(argv[3]),std::stoi(argv[4]),std::stoi(argv[5]));
    return 0;
}

Dialog *Dialog::m_edit_dialog = nullptr;
std::list<Dialog> *Dialog::m_dialogs = nullptr;
std::vector<User> *Dialog::m_users = nullptr;
}  // namespace database_interface