//
// Created by andrey on 15.03.23.
//

#include "NetClient.hpp"
#include <unistd.h>

int main() {
    Net::Client::Client client("localhost", "12345");
    client.make_connection();
    for (int i = 0; i < 10; ++i) {
        client.send_text_message("Hi, that is iteration № " + std::to_string(i) + "!");
        client.get_request_and_out_it();
        usleep(100'000);
    }
}
