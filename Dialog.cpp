#include "include/Dialog.hpp"

namespace database_interface {

int Dialog::callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i+=6){
        m_dialog_list->insert(m_dialog_list.end(),
                            Dialog(std::stoi(argv[i]),
                                   argv[i+1],
                                   argv[i+2],
                                   std::stoi(argv[i+3]),
                                   std::stoi(argv[i+4]),
                                   std::stoi(argv[i+5])));
    }
    return 0;
}

std::list<Dialog> *Dialog::m_dialog_list = nullptr;
}  // namespace database_interface