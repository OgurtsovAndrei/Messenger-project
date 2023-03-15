//
// Created by andrey on 08.03.23.
//


#include "NetServer.hpp""

int main() {
    Net::Server::Server server(12345);
    server.run_server();

    /*Net::Request first(Net::TEXT_MESSAGE, "AbaCaba");
    first.make_request();
    std::cout << first.get_text_request() << "\n";
    Net::Request second(first.get_text_request(), true);
    second.parse_request();
    std::cout << second.get_body() << "\n";*/

//    Net::Client::Client client;
//    client.make_connection();

}
