//
// Created by andrey on 15.03.23.
//

#define MULTI_CLIENT_TEST

#include "NetClient.hpp"
#include <unistd.h>

//...
int main() {
    boost::asio::io_context io_context;
//    std::vector<Net::Client::Client> client_vec;
//    client_vec.reserve(100);

    std::vector<std::thread> ts;
    for (int i = 0; i < 1000; ++i) {
        ts.emplace_back([&io_context]() {
            Net::Client::Client client(io_context);
            client.make_connection();
            for (int i = 0; i < 10; ++i) {
                client.send_text_message("Hi, that is iteration № " + std::to_string(i) + "!");
                client.get_request_and_out_it();
                usleep(100'000);
            }
        });
        usleep(5'000);
//        client_vec.emplace_back(Net::Client::Client(io_context));
//        Net::Client::Client &client = client_vec.back();
    }
    for (auto &t : ts) {
        t.join();
    }
}
