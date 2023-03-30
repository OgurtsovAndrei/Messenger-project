//
// Created by andrey on 15.03.23.
//

#include "NetClient.hpp"
#include <unistd.h>

int main() {
    Net::Client::Client client("localhost", "12345");
//    client.make_connection();
    client.make_secure_connection();
    for (int i = 0; i < 3; ++i) {
        std::cout << "Iteration #" << i << "\n";
        client.send_text_message("Hi, that is iteration № " + std::to_string(i) + "!");
        client.get_request_and_out_it();
        client.send_secured_text_message("SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!");
        client.get_secret_request_and_out_it();
        usleep(100'000);
    }
}
