#include "include/Message.hpp"

namespace database_interface {

int Message::callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i+=5){
        m_message_list->insert(m_Message_list.end(),
                              Message(std::stoi(argv[i]),
                                      std::stoi(argv[i+1]),
                                     argv[i+2],
                                     argv[i+3],
                                     std::stoi(argv[i+5])));
    }
    return 0;
}

std::list<Message> *Message::m_message_list = nullptr;
}  // namespace database_interface