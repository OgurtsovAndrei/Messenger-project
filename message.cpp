#include "message.hpp"

namespace database_interface {

int Message::callback_get_message_by_id(void *NotUsed, int argc, char **argv, char **azColName) {
    if (argc < 6){
        return 0;
    }
    *m_edit_message = Message(std::stoi(argv[0]),
                              std::stoi(argv[1]),
                              argv[2],
                              argv[3],
                              std::stoi(argv[4]),
                              std::stoi(argv[5]));
    return 0;
}

int Message::callback_get_message_list(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i+=5){
        m_message_list->push_back(Message(std::stoi(argv[i]),
                                          std::stoi(argv[i+1]),
                                          argv[i+2],
                                          argv[i+3],
                                          std::stoi(argv[i+5])));
    }
    return 0;
}

Message *Message::m_edit_message = nullptr;
std::list<Message> *Message::m_message_list = nullptr;
}  // namespace database_interface