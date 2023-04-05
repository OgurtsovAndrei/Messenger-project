//
// Created by andrey on 08.03.23.
//

#include "NetServer.hpp"
#include <cassert>

int main() {
    Net::Server::Server server(12345);
    auto open_res = server.open_database();
    if (!open_res.correct()) {
        std::cout << open_res.message() << "\n";
        return 0;
    }
    server.run_server(4);
    assert(server.close_database().correct());
    return 0;
}
