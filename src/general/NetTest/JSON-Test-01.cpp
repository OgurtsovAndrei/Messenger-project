//
// Created by andrey on 30.04.23.
//

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Message {
    int m_message_id;
    int m_date_time;
    std::string m_text;
    int m_user_id;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Message, m_message_id, m_date_time, m_text, m_user_id);
};

int main() {
//    std:: string string = "{\n"
//                     "  \"pi\": 3.141,\n"
//                     "  \"happy\": true,\n"
//                     "  \"name\": \"Niels\",\n"
//                     "  \"nothing\": null,\n"
//                     "  \"answer\": {\n"
//                     "    \"everything\": 42\n"
//                     "  },\n"
//                     "  \"list\": [1, 0, 2],\n"
//                     "  \"object\": {\n"
//                     "    \"currency\": \"USD\",\n"
//                     "    \"value\": 42.99\n"
//                     "  }\n"
//                     "}";
//
////std::ifstream f("example.json");
////std::ifstream f("example.json");
//    Message message{7, 13, "lalala", 21};
//    json msg = message;
//
//    json data = json::parse(string);
//    std::cout << data["pi"] << std::endl;
//    std::cout << data.size() << std::endl;
//    auto nl = data["SmthNotInData"];
//    assert(nl.is_null());
//    data["MyRandomNumber"] = 239566;
//    std::cout << data.dump() << std::endl;
//    std::cout << msg.dump(-1) << std::endl;
//    std::string str = msg.dump(4);

    Message new_message{0, 0, "123", 0};
    json message = new_message;
    json other_msg = R"({"m_date_time":13,"m_message_id":7,"m_user_id":21})"_json;
    try {
        nlohmann::from_json(other_msg, new_message);
    } catch (...) {
        std::cerr << "Bad json!\n";
    }
    std::cout << new_message.m_user_id << std::endl;
    std::cout << new_message.m_text << std::endl;
    std::cout << new_message.m_date_time << std::endl;
    std::cout << new_message.m_message_id << std::endl;
}



