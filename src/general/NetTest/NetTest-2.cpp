//
// Created by andrey on 15.03.23.
//

#include "NetClient.hpp"
#include "NetGeneral.hpp"
#include "../../../include/TextWorker.hpp"
#include <unistd.h>

int main() {
    Net::Client::Client client("localhost", "12345");
//    client.make_connection();
    client.make_secure_connection();
    {
        auto status = client.log_in("A-login", "A-password");
        if (status) {
            std::cout << "Logged in -->>" + status.message() + "\n";
        } else {
            std::cout << "Log in failed -->> " + status.message() + "\n";
            return 0;
        }
    }
    for (int i = 0; i < 3; ++i) {
        std::cout << "Iteration #" << i << "\n";
        client.send_secured_text_request("SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!");
        client.get_secret_request_and_out_it();
        usleep(100'000);
    }
    auto pair = client.get_last_n_dialogs(100);
    if (pair.first) {
        for (const auto& dialog : pair.second) {
            std::cout << "Dialog in str-view: " << dialog.to_string() << "\n";
        }
    }
    {
        auto [status, user] = client.get_user_id_by_login("A-login");
        if (status) {
            std::cout << "User id: " << user.m_user_id << "\tExpected id: ___\n";
            std::cout << "User login: " << user.m_login << "\tExpected login: A-login\n";
            std::cout << "User name: " << user.m_name << "\tExpected name: ___\n";
            std::cout << "User surname: " << user.m_surname << "\tExpected surname: ___\n";
//            assert(std::vector<std::string>{user.} == std::vector<std::string>{})
        } else {
            std::cout << status.message();
        }
    }
}
