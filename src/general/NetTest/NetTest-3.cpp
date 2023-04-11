//
// Created by andrey on 15.03.23.
//

#define MULTI_CLIENT_TEST

#include "NetClient.hpp"
#include <unistd.h>
#include <chrono>

//...
int main() {
    boost::asio::io_context io_context;
    std::vector<std::thread> ts;
    int n_of_workers = 100;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n_of_workers; ++i) {
        ts.emplace_back([&io_context]() {
            Net::Client::Client client(io_context);

            client.make_unsecure_connection();
            for (int i = 0; i < 10; ++i) {
                client.send_text_request("Hi, that is iteration № " + std::to_string(i) + "!");
                client.get_request_and_out_it();
                usleep(100'000);
            }

        });
        usleep(5'000);
    }
    for (auto &t: ts) { t.join(); }
    ts.clear();
    auto start_secure = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < n_of_workers; ++i) {
        ts.emplace_back([&io_context]() {
            Net::Client::Client client(io_context);
            client.make_secure_connection();
            for (int i = 0; i < 10; ++i) {
                std::cout << "Iteration #" << i << "\n";
                client.send_text_request("Hi, that is iteration № " + std::to_string(i) + "!");
                client.get_request_and_out_it();
                client.send_secured_text_request("SECURED --> Secret hi, from iteration № " + std::to_string(i) + "!");
                client.get_secret_request_and_return_body();
                usleep(100'000);
            }
        });
        usleep(5'000);
    }
    for (auto &t: ts) { t.join(); }
    auto stop = std::chrono::high_resolution_clock::now();
    auto whole_duration = duration_cast<std::chrono::milliseconds>(stop - start);
    auto secured_duration = duration_cast<std::chrono::milliseconds>(stop - start_secure);
    std::cout << "Whole process takes " << 0.001 * whole_duration.

            count()

              << " ms" <<
              std::endl;
    std::cout << "Secured part takes " << 0.001 * secured_duration.

            count()

              << " ms" <<
              std::endl;
}
