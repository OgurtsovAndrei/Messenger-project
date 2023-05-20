//
// Created by andrey on 15.03.23.
//

#include <cassert>
#include "Net/NetClient.hpp"
#include "Net/NetGeneral.hpp"
#include "TextWorker.hpp"
#include <unistd.h>

int main() {
    Net::Client::Client client("localhost", "12345");
    client.make_secure_connection();
    std::string login = "New-Login-312";
    std::string password =  "New-Password-123";
    std::string surname = "New-Surname-123";
    std::string name = "New-username-123";\
    auto status = client.log_in(login, password);
    std::cout << "Log in status is: " << (status ? "success" : "fail") << " with user id = : " << status.message() << std::endl;
    client.close_connection();
}
