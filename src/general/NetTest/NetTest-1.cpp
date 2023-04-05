//
// Created by andrey on 08.03.23.
//

#include "NetServer.hpp"
#include <cassert>

int main() {
    Net::Server::Server server(12345);
    assert(server.open_database());
    server.run_server(4);
    assert(server.close_database());
    return 0;
}
