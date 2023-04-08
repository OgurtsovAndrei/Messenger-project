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
#include "./../../../include/Status.hpp"



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

        void make_unsecure_connection() {
            // Establish connection
            boost::asio::ip::tcp::socket s(io_context);
            boost::asio::connect(s, boost::asio::ip::tcp::resolver(io_context).resolve(server_ip, server_port));
            connection = boost::asio::ip::tcp::iostream(std::move(s));
            // Ensure that connection in unsecure
            send_request(MAKE_UNSECURE_CONNECTION, "");
            Request request = accept_request(connection.value());
            request.parse_request();
            assert(request);
            assert(request.get_type() == MAKE_UNSECURE_CONNECTION_SUCCESS);
            std::cout << "Unsecure connection wa established!\n";
        }

        void make_secure_connection() {
            // Establish connection
            boost::asio::ip::tcp::socket client_socket(io_context);
            boost::asio::connect(client_socket, boost::asio::ip::tcp::resolver(io_context).resolve(server_ip, server_port));
            connection = boost::asio::ip::tcp::iostream(std::move(client_socket));
            // Create decrypter and send public key
            decrypter = Cryptographer::Decrypter(cryptographer.get_rng());
            send_request(MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY, decrypter.value().get_str_publicKey());
            // Get other public key
            Request request = accept_request(connection.value());
            request.parse_request();
            assert(request);
            assert(request.get_type() == MAKE_SECURE_CONNECTION_SUCCESS_RETURN_OTHER_KEY);
            encrypter = Cryptographer::Encrypter(request.get_body(), cryptographer.get_rng());
            send_request(MAKE_SECURE_CONNECTION_SUCCESS, "");
            connection_is_secured = true;
            std::cout << "Secured connection was established!\n";
        }

        void send_request(RequestType type, std::string message) {
            send_message_by_connection(type, std::move(message), connection.value());
        }

        void send_secured_request(RequestType type, const std::string& message) {
            assert(connection_is_secured);
            std::string encrypted_message = encrypter.value().encrypt_text_to_text(message);
            send_message_by_connection(type, std::move(encrypted_message), connection.value());
        }

        void send_text_request(std::string message) {
            send_request(TEXT_REQUEST, std::move(message));
        }

        void send_secured_text_request(const std::string& message) {
            assert(connection_is_secured);
            std::string encrypted_message = encrypter.value().encrypt_text_to_text(message);
            send_request(SECURED_REQUEST, std::move(encrypted_message));
        }

        void get_request_and_out_it() {
            Request request = accept_request(connection.value());
            auto true_string = request.get_body();
            std::cout << "Got from server: " << true_string << "\n";
        }

        Request get_request() {
            Request request = accept_request(connection.value());
            return std::move(request);
        }

        Request decrypt_request(Request request) { //NOLINT [static_method]
            auto decrypted_body = Cryptographer::as<std::string>(
                    decrypter.value().decrypt_data(request.get_body()));
            request.set_body(decrypted_body);
            return std::move(request);
        }

        void get_secret_request_and_out_it() {
            Request request = accept_request(connection.value());
            auto decrypted_body = Cryptographer::as<std::string>(
                    decrypter.value().decrypt_data(request.get_body()));
            request.set_body(decrypted_body);
            std::cout << "Got from server: " << decrypted_body << "\n";
        }

        Status send_message_to_another_user(int dialog_id, int current_time, std::string text) {
            std::vector<std::string> data_vector {std::to_string(dialog_id), std::to_string(current_time), std::move(text)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(SEND_MESSAGE, data_to_send);
            } else {
                send_request(SEND_MESSAGE, data_to_send);
            }
            Request response = get_request();
            response.parse_request();
            if (response && response.get_type() == SEND_MESSAGE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == SEND_MESSAGE_FAIL);
                if (connection_is_secured) {
                    response = decrypt_request(std::move(response));
                }
                return Status(false, response.get_body());
            }
        }

        Status change_sent_message(int old_message_id, std::string new_text) {
            // We change text (message body) in old message, we get by id, to new_text.
            std::vector<std::string> data_vector {std::to_string(old_message_id), std::move(new_text)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(CHANGE_MESSAGE, data_to_send);
            } else {
                send_request(CHANGE_MESSAGE, data_to_send);
            }
            Request response = get_request();
            if (response && response.get_type() == CHANGE_MESSAGE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == CHANGE_MESSAGE_FAIL);
                if (connection_is_secured) {
                    response = decrypt_request(std::move(response));
                }
                return Status(false, response.get_body());
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
        bool connection_is_secured = false;
        Cryptographer::Cryptographer cryptographer;
        std::optional<Cryptographer::Encrypter> encrypter;
        std::optional<Cryptographer::Decrypter> decrypter;
    };

}

#endif //MESSENGER_PROJECT_NETCLIENT_HPP
