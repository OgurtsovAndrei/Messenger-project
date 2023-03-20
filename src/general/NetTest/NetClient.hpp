//
// Created by andrey on 15.03.23.
//

#ifndef MESSENGER_PROJECT_NETCLIENT_HPP
#define MESSENGER_PROJECT_NETCLIENT_HPP

#include <utility>
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <random>
#include <mutex>
#include <optional>

#include "NetGeneral.hpp"
#include "./../CryptoTest/Cryptographer.hpp"


namespace Net::Client {

    struct Client {
    public:
#ifndef MULTI_CLIENT_TEST

        Client(std::string server_ip_ = "localhost", std::string port_ = "12345") :
                server_ip(std::move(server_ip_)), server_port(std::move(port_)) {};
#else
        Client(boost::asio::io_context &ioContext, std::string server_ip_ = "localhost", std::string port_ = "12346") :
                io_context(ioContext), server_ip(std::move(server_ip_)), server_port(std::move(port_)) {};
#endif

        void make_connection() {
            boost::asio::ip::tcp::socket s(io_context);
            boost::asio::connect(s, boost::asio::ip::tcp::resolver(io_context).resolve(server_ip, server_port));
            connection = boost::asio::ip::tcp::iostream(std::move(s));
        }

        void send_text_message(std::string message) {
            send_message_by_connection(TEXT_MESSAGE, std::move(message), connection.value());
        }

        void send_message(RequestType type, std::string message) {
            send_message_by_connection(type, std::move(message), connection.value());
        }

        void print_line_from_connection() {
            std::cout << get_line_from_connection(connection.value()) << "\n";
        }

        void get_request_and_out_it() {
            Request request = accept_request(connection.value());
            auto true_string = request.get_body();
            std::cout << "Got from server: " << true_string << "\n";
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
