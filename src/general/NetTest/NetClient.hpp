//
// Created by andrey on 15.03.23.
//

#ifndef MESSENGER_PROJECT_NETCLIENT_HPP
#define MESSENGER_PROJECT_NETCLIENT_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <utility>
#include <thread>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <random>
#include <mutex>
#include <optional>

#include "NetGeneral.hpp"


namespace Net::Client {

    struct Client {
    public:
#ifndef MULTI_CLIENT_TEST
        Client(std::string server_ip_ = "localhost", std::string port_ = "12345") :
        server_ip(std::move(server_ip_)), server_port(std::move(port_)) {};
#else
        Client(boost::asio::io_context &ioContext, std::string server_ip_ = "localhost", std::string port_ = "12345") :
                io_context(ioContext), server_ip(std::move(server_ip_)), server_port(std::move(port_)) {};
#endif

        void make_connection() {
            boost::asio::ip::tcp::socket s(io_context);
            boost::asio::connect(s, boost::asio::ip::tcp::resolver(io_context).resolve(server_ip, server_port));
            connection = boost::asio::ip::tcp::iostream(std::move(s));
        }

        void send_text_to_server(const std::string &text) {
            connection.value() << text << std::endl;
        }

        void send_text_message(std::string message) {
            Request request(TEXT_MESSAGE, std::move(message));
            request.make_request();
            if (request) {
                send_text_to_server(request.get_text_request());
//                connection.value() << request.get_text_request() << std::endl;
                std::string response;
                std::getline(connection.value(), response);
                std::cout << response << "\n";
            }
        }

    private:
#ifndef MULTI_CLIENT_TEST
        boost::asio::io_context io_context;
#else
        boost::asio::io_context &io_context;
#endif
        std::optional<boost::asio::ip::tcp::iostream> connection;
        std::string server_ip;
        std::string server_port;
    };

}

#endif //MESSENGER_PROJECT_NETCLIENT_HPP
