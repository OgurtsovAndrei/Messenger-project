//
// Created by andrey on 15.03.23.
//

#include <cassert>
#include "NetClient.hpp"
#include "NetGeneral.hpp"
#include "../../../include/TextWorker.hpp"
#include <unistd.h>

int main() {
    Net::Client::Client client("localhost", "12345");
    client.make_secure_connection();
    if (false) {
//        std::cout << "Done0" << std::endl;
        auto status1 = client.log_in("sdfasdf", "asdf");
        assert(!status1);
//        std::cout << "Done1" << std::endl;
        auto status2 = client.log_in("A-login", "asdf");
        assert(!status2);
//        std::cout << "Done2" << std::endl;
        auto status3 = client.log_in("asdfadgin", "A-password");
        assert(!status3);
//        std::cout << "Done3" << std::endl;
        auto status = client.log_in("A-login", "A-password");
        assert(status);
//        std::cout << "Done4" << std::endl;
        if (status) {
            std::cout << "Logged in -->>" + status.message() + "\n";
        } else {
            std::cout << "Log in failed -->> " + status.message() + "\n";
            return 0;
        }
    }
    if (true) {
        std::string login = "New-Login-1";
        std::string password =  "New-Password-1";
        std::string surname = "New-Surname-1";
        std::string name = "New-username-1";
        auto status = client.sing_up(name, surname, login, password);
        std::cout << "Signing up status is: " << (status ? "success" : "fail") << " with message: " << status.message() << std::endl;
        status = client.log_in(login, password);
        std::cout << "Signing up status is: " << (status ? "success" : "fail") << " with message: " << status.message() << std::endl;

    }
    for (int i = 0; i < 3; ++i) {
        std::cout << "Iteration #" << i << "\n";
        client.send_secured_text_request("SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!");
        assert(client.get_secret_request_and_return_body() == "Got from server: Got from you: <SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!>");
        usleep(100'000);
    }
    auto pair = client.get_last_n_dialogs(100);
    if (pair.first) {
        for (const auto &dialog: pair.second) {
            std::cout << "Dialog in str-view: " << dialog.to_string() << "\n";
        }
    }
    {
        auto [status, user] = client.get_user_id_by_login("A-login");
        if (status) {
//            std::cout << "User id: " << user.m_user_id << "\tExpected id: ___\n";
//            std::cout << "User login: " << user.m_login << "\tExpected login: A-login\n";
//            std::cout << "User name: " << user.m_name << "\tExpected name: ___\n";
//            std::cout << "User surname: " << user.m_surname << "\tExpected surname: ___\n";
            std::vector<std::string> given{std::to_string(user.m_user_id), user.m_login, user.m_name, user.m_surname};
            std::vector<std::string> expected{"2", "A-login", "A-name", "A-surname"};
            assert(given == expected);
        } else {
            std::cout << status.message();
        }
    }
    if (false) {
        std::cout << "Before send: -------------------------------------- \n";
        auto [message_status, messages] = client.get_n_messages(100, 1, INT32_MAX);
        assert(message_status);
        for (const auto& message : messages) {
            std::cout << "Message: " << message.m_text << "\n";
        }
        std::cout << "After send: -------------------------------------- \n";
        Status send_status = client.send_message_to_another_user(1, 1234567, "A-to-B - Test message!");
        database_interface::Message sent_message{};
        database_interface::Message::parse_to_message(send_status.message(), sent_message);
        std::cout << "Sent message id: " << sent_message.m_message_id << std::endl;
        std::cout << "Sent message: " << sent_message.m_text << " " << sent_message.m_user_id << std::endl;
        auto [new_message_status, new_messages] = client.get_n_messages(100, 1, INT32_MAX);
        assert(new_message_status);
        for (const auto& message : new_messages) {
            std::cout << "Message: " << message.m_text << "\n";
        }
        std::cout << "After delete: -------------------------------------- \n";
        Status delete_status = client.delete_message(sent_message.m_message_id);
        assert(delete_status);
        auto [after_delete_message_status, after_delete_messages] = client.get_n_messages(100, 1, INT32_MAX);
        assert(after_delete_message_status);
        for (const auto& message : after_delete_messages) {
            std::cout << "Message: " << message.m_text << "\n";
        }
        std::cout << "---------------------------------------------------- \n";
    }
    client.close_connection();
}
