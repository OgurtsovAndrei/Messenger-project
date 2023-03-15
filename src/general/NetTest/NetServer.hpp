//
// Created by andrey on 08.03.23.
//

#ifndef MESSENGER_PROJECT_NETSERVER_HPP
#define MESSENGER_PROJECT_NETSERVER_HPP

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

namespace Net::Server {

    struct Server;

    struct UserConnection {

    public:
        UserConnection(const UserConnection &con) = delete;
        UserConnection(UserConnection &&) = default;
        UserConnection &operator=(const UserConnection &) = delete;
        UserConnection &operator=(UserConnection &&) = default;

        explicit UserConnection(boost::asio::ip::tcp::socket &&socket_, Server &server_, int connection_number_) :
                server(server_), connection_number(connection_number_), current_socket(std::move(socket_)) {};

        void work(boost::asio::ip::tcp::socket &&socket);

        void accept() {
            session_thread = std::move(std::thread([socket = std::move(current_socket), this]() mutable {
                work(std::move(socket));
            }));
        }

        ~UserConnection() {
            if (session_thread) {
//                session_thread.value().join();
                session_thread->detach();
            }
        }

    private:
        int connection_number;
        std::optional<std::thread> session_thread;
        boost::asio::ip::tcp::socket current_socket;
        Server &server;
        // TODO: some encrypting keys
    };

    struct Server {
    public:
        explicit Server(const int port_ = 80) : port(port_), connection_acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_)) {
            std::cout << "Listening at " << connection_acceptor.local_endpoint() << "\n";
        }

        [[nodiscard]] [[maybe_unused]] int get_port() const {
            return port;
        }

        [[noreturn]] void run_server() {
            while (true) {
                make_connection();
            }
        }

        void run_server_thread() {
            /*
             * TODO
             * FIXME
             * При вызове данной функции всё падает, я не знаю, в чём проблема
             * Мне нужна помощь тут
             * run_server() прекрасно работает, но отжирает весь main thread 😥😭
             */
            server_thread = std::move(std::thread([&, this]() mutable {
                while (true) {
                    make_connection();
                }
            }));
        }

        /*static std::thread run_server_in_other_thread(Server &&server) {
            auto server_thread = std::move(std::thread(
                    [lambda_server = std::move(server)]() mutable {

            }));
        }*/

        void make_connection() {
            boost::asio::ip::tcp::socket socket = connection_acceptor.accept();
            Server &us = *this;
            int empty_number = find_empty_connection_number();
            sessions.emplace(empty_number, UserConnection(std::move(socket), us, empty_number));
            UserConnection &connection = sessions.find(empty_number)->second;
            connection.accept();
        }

        void close_connection(int connection_number) {
            assert(sessions.count(connection_number));
            sessions.erase(connection_number);
        }

    private:
        std::optional<std::thread> server_thread;
        friend struct UserConnection;
        boost::asio::io_context io_context;
        boost::asio::ip::tcp::acceptor connection_acceptor;
        const int port;
        std::mutex sessions_mutex;
        std::unordered_map<int, UserConnection> sessions;

        int find_empty_connection_number() {
            std::mt19937 rng((uint32_t) std::chrono::steady_clock::now().time_since_epoch().count());
            int val;
            std::unique_lock lock(sessions_mutex);
            while (true) {
                std::uniform_int_distribution<> gen_int(0, 2 << 29);
                val = gen_int(rng);
                if (sessions.count(val) == 0) {
                    break;
                }
            }
            return val;
        }
    };

    void accept_request(boost::asio::ip::tcp::iostream &client, const std::string& rem_endpoint_str) {
        // TODO Очень плохой код! Починить!
        // Но оно работает...

        /*std::string s;
        if (!(client >> s)) {
            return;
        }
        char c;
        while (c = std::getchar(std::cin, c)) {

        }*/
        char str[2];
        std::string true_string;
        bool flag = true;
        while (flag) {
            client.read(str, 1);
            true_string.push_back(str[0]);
            if (true_string.find(request_begin) != std::string::npos) {
                flag = false;
            }
            if (true_string.size() >= 20) {
                true_string = true_string.substr(10, true_string.size()-10);
            }
        }
        assert(true_string.find(request_begin) == true_string.size() - 5);
        true_string = request_begin;
        while (true_string.find(request_sep) == std::string::npos) {
            client.read(str, 1);
            true_string.push_back(str[0]);
        }
        assert(true_string.find(request_sep) == true_string.size() - 5);

        std::string new_part;
        while (new_part.find(request_sep) == std::string::npos) {
            client.read(str, 1);
            new_part.push_back(str[0]);
        }
        assert(new_part.find(request_sep) == new_part.size() - 5);
        true_string += new_part;

        // Careful!
        int body_size = std::stoi(new_part.substr(0, new_part.size() - 5));
        char *body_ptr = new char[body_size + 1];
        client.read(body_ptr, body_size);
        true_string += body_ptr;
        delete[] body_ptr;

        char last_request_part[6];
        client.read(last_request_part, 5);
        true_string += last_request_part;
        assert(true_string.find(request_end) == true_string.size() - 5);

        std::cout << "Dot request from" << rem_endpoint_str << ": " << true_string << "\n";
        Request request(true_string, true);
        request.parse_request();
        true_string = request.get_body();
        std::cout << "got from " << rem_endpoint_str << ": " << true_string << "\n";
        client << "got from you: " << true_string << "\n";
    }

    void UserConnection::work(boost::asio::ip::tcp::socket &&socket) {
        std::cout << "Accepted connection " << socket.remote_endpoint() << " --> "
                  << socket.local_endpoint() << "\n";
        auto rem_endpoint = socket.remote_endpoint();
        boost::asio::ip::tcp::iostream client(std::move(socket));
        while (client) {
            accept_request(client, rem_endpoint.address().to_string());
//            accept_request(client, static_cast<std::string>(rem_endpoint.address()));
        }
        std::cout << "Completed -> " << rem_endpoint << "\n";
        // AWARE func calls detach!
        server.close_connection(connection_number);
    }

}

#endif //MESSENGER_PROJECT_NETSERVER_HPP
