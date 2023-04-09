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
    auto status = client.log_in("A-login", "A-password");
    if (status) {
        std::cout << "Logged in -->>" + status.message() + "\n";
    } else {
        std::cout << "Log in failed -->>" + status.message() + "\n";
    }
    for (int i = 0; i < 3; ++i) {
        std::cout << "Iteration #" << i << "\n";
        client.send_secured_text_request("SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!");
        client.get_secret_request_and_out_it();
        usleep(100'000);
    }
}
