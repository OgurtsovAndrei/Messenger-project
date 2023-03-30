//
// Created by andrey on 08.03.23.
//

#include "NetServer.hpp"

int main() {
    Net::Server::Server server(12345);
    server.run_server(4);
}
