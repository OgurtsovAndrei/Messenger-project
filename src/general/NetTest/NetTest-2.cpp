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
    std::vector<std::string> user_data = {"A-login", "A-password"};
    client.send_request(Net::LOG_IN_REQUEST, convert_text_vector_to_text(user_data));
    auto request = client.get_request();
    if (request.get_type() == Net::LOG_IN_SUCCESS) {
        std::cout << "Loging success!\n";
    } else {
        std::cout << "Loging fail!\n";
        return 0;
    }
    for (int i = 0; i < 3; ++i) {
        std::cout << "Iteration #" << i << "\n";
        client.send_text_request("Hi, that is iteration № " + std::to_string(i) + "!");
        client.get_request_and_out_it();
        client.send_secured_text_request("SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!");
        client.get_secret_request_and_out_it();
        usleep(100'000);
    }
}
