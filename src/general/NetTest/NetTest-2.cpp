//
// Created by andrey on 15.03.23.
//

#include <cassert>
#include "Net/NetClient.hpp"
#include "Net/NetGeneral.hpp"
#include "TextWorker.hpp"
#include <unistd.h>

int main() {
    Net::Client::Client client("localhost", "12345");
    client.make_secure_connection();
    if (false) {
        std::string login = "New-Login-1";
        std::string password =  "New-Password-1";
        std::string surname = "New-Surname-1";
        std::string name = "New-username-1";
        std::cout << "Done0" << std::endl;
        auto status1 = client.log_in("sdfasdf", "asdf").first;
        assert(!status1);
        std::cout << "Done1" << std::endl;
        auto status2 = client.log_in("A-login", "asdf").first;
        assert(!status2);
        std::cout << "Done2" << std::endl;
        auto status3 = client.log_in("asdfadgin", "A-password").first;
        assert(!status3);
        std::cout << "Done3" << std::endl;
        auto status = client.log_in("A-login", "A-password").first;
        assert(status);
        std::cout << "Done4" << std::endl;
        if (status) {
            std::cout << "Logged in -->>" + status.message() + "\n";
        } else {
            std::cout << "Log in failed -->> " + status.message() + "\n";
            return 0;
        }
    } else {
        std::string login = "New-Login-239566";
        std::string password =  "New-Password-1";
        std::string surname = "New-Surname-1";
        std::string name = "New-username-1";
        auto status = client.sign_up(name, surname, login, password);
        std::cout << "Signing up status is: " << (status ? "success" : "fail") << " with message: " << status.message() << std::endl;
        status = client.log_in(login, password).first;
        std::cout << "Log in status is: " << (status ? "success" : "fail") << " with user id = : " << status.message() << std::endl;
    }
    for (int i = 0; i < 3; ++i) {
        std::cout << "Iteration #" << i << "\n";
        auto resp = client.send_text_request_and_return_response_text("SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!");
        if (resp != "Got from you: <SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!>") {
            std::cerr << "<" << resp << ">\n";
            assert(resp == "Got from you: <SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!>");
        }
        usleep(100'000);
    }
    if (false) {
        auto pair = client.get_last_n_dialogs(100);
        if (pair.first) {
            for (const auto &dialog: pair.second) {
                std::cout << "Dialog in str-view: " << static_cast<json>(dialog).dump(4) << "\n";
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
        if (true) {
            std::cout << "Before send: -------------------------------------- \n";
            auto [message_status, messages] = client.get_n_messages(100, 1, INT32_MAX);
            assert(message_status);
            for (const auto& message : messages) {
                std::cout << "Message: " << message.m_text << "\n";
            }
            std::cout << "After send: -------------------------------------- \n";
            Status send_status = client.send_message_to_another_user(1, 1234567, "A-to-B - Test message!");
            database_interface::Message sent_message = json::parse(send_status.message());
            std::cout << "Sent message id: " << sent_message.m_message_id << std::endl;
            std::cout << "Sent message: " << sent_message.m_text << " " << sent_message.m_user_id << std::endl;
            auto [new_message_status, new_messages] = client.get_n_messages(100, 1, INT32_MAX);
            assert(new_message_status);
            for (const auto& message : new_messages) {
                std::cout << "Message: \n" << json{message}.dump(-1) << "\n";
            }
            std::cout << "After delete: -------------------------------------- \n";
            Status delete_status = client.delete_message(sent_message.m_message_id);
            assert(delete_status);
            auto [after_delete_message_status, after_delete_messages] = client.get_n_messages(100, 1, INT32_MAX);
            assert(after_delete_message_status);
            for (const auto& message : after_delete_messages) {
                std::cout << "Message: \n" << json{message}.dump(-1) << "\n";
            }
            std::cout << "---------------------------------------------------- \n";
        }
    }

    if (false) {
        std::cout << "---------- File section ----------" << std::endl;
        FileWorker::File file(FileWorker::empty_file);
        try {
            file = FileWorker::File("./../bd/Files/Lena.bmp");
        } catch (FileWorker::file_exception &exception) {
            std::cerr << exception.what() << std::endl;
            return 0;
        }
        std::cout << "File was read!\n";
        file.change_name("Lena_sent.bmp");
        std::cout << "File name was changes!\n";
        Status send_status = client.upload_file(file);
        std::cout << "File was sent!\n";
        if (send_status) {
            std::cout << "Sent file name = " + send_status.message() << std::endl;
        } else {
            std::cerr << send_status.message() << std::endl;
            goto file_section;
        }
        {
            std::cout << "Sending receive file request!\n";
            auto [receive_status, received_file] = client.download_file(send_status.message());
            received_file.change_name("Lena_after_receive.bmp");
            Status save_status = received_file.save("./../bd/Files/saved");
            std::cout << "File was received!\n";
            assert(save_status);
        }
        file_section:;
    }

    if (true) {
        std::cout << "========== Det users in dialog section ==========\n";
        int dialog_id = 3;
        auto [status, users] = client.get_users_in_dialog(dialog_id);
        if (status) {
            std::cout << "Users in dialog with id: " + std::to_string(dialog_id) + "" << std::endl;
            for (const auto& user : users) {
                std::cout << json(user).dump(3) << std::endl;
            }
        } else {
            std::cerr << status.message() << std::endl;
        }
    }
    client.close_connection();
    client.make_secure_connection();
    if (false){
        std::cout << "==================Change dialog==================\n";
        std::string login = "C6";
        std::string password =  "C";
        std::string surname = "C";
        std::string name = "C";
        auto status = client.sign_up(name, surname, login, password);
        std::cout << "Signing up status is: " << (status ? "success" : "fail") << " with message: " << status.message() << std::endl;
        status = client.log_in(login, password).first;
        std::cout << "Signing up status is: " << (status ? "success" : "fail") << " with message: " << status.message() << std::endl;
        status = client.change_user(22, "encryption", "encryption", 3).first;
        std::cout << "Signing up status is: " << (status ? "success" : "fail") << " with message: " << status.message() << std::endl;
    }
    client.close_connection();
}
