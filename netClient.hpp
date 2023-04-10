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

#include "netGeneral.hpp"
#include "cryptographer.hpp"
#include "message.hpp"
#include "dialog.hpp"
#include "status.hpp"


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
            Request request = accept_request(connection.value(), connection_is_secured);
            request.parse_request();
            assert(request);
            assert(request.get_type() == MAKE_UNSECURE_CONNECTION_SUCCESS);
            std::cout << "Unsecure connection wa established!\n";
        }

        void make_secure_connection() {
            // Establish connection
            boost::asio::ip::tcp::socket client_socket(io_context);
            boost::asio::connect(client_socket,
                                 boost::asio::ip::tcp::resolver(io_context).resolve(server_ip, server_port));
            connection = boost::asio::ip::tcp::iostream(std::move(client_socket));
            // Create decrypter and send public key
            decrypter = Cryptographer::Decrypter(cryptographer.get_rng());
            send_request(MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY, decrypter.value().get_str_publicKey());
            // Get other public key
            Request request = accept_request(connection.value(), connection_is_secured);
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

        void send_secured_request(RequestType type, const std::string &message) {
            assert(connection_is_secured);
            std::string encrypted_message = encrypter.value().encrypt_text_to_text(message);
            send_message_by_connection(type, std::move(encrypted_message), connection.value());
        }

        void send_text_request(std::string message) {
            assert(!connection_is_secured);
            send_request(TEXT_REQUEST, std::move(message));
        }

        void send_secured_text_request(const std::string &message) {
            assert(connection_is_secured);
            std::string encrypted_message = encrypter.value().encrypt_text_to_text(message);
            send_request(SECURED_REQUEST, std::move(encrypted_message));
        }

        void get_request_and_out_it() {
            Request request = accept_request(connection.value(), connection_is_secured);
            auto true_string = request.get_body();
            std::cout << "Got from server: " << true_string << "\n";
        }

        Status send_message_to_another_user(int dialog_id, int current_time, std::string text) {
            std::vector<std::string> data_vector{std::to_string(dialog_id), std::to_string(current_time),
                                                 std::move(text)};
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
            std::vector<std::string> data_vector{std::to_string(old_message_id), std::move(new_text)};
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

        Status delete_message(int message_id) {
            // We change text (message body) in old message, we get by id, to new_text.
            std::vector<std::string> data_vector{std::to_string(message_id)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(DELETE_MESSAGE, data_to_send);
            } else {
                send_request(DELETE_MESSAGE, data_to_send);
            }
            Request response = get_request();
            if (response && response.get_type() == DELETE_MESSAGE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == DELETE_MESSAGE_FAIL);
                if (connection_is_secured) {
                    response = decrypt_request(std::move(response));
                }
                return Status(false, response.get_body());
            }
        }

        std::pair<Status, std::vector<database_interface::Message>>
        get_n_messages(int n, int dialog_id, int last_message_time = -1) {
            std::vector<std::string> data_vector{std::to_string(n), std::to_string(dialog_id),
                                                 std::to_string(last_message_time)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(GET_100_MESSAGES, data_to_send);
            } else {
                send_request(GET_100_MESSAGES, data_to_send);
            }
            Request response = get_request();
            if (response && response.get_type() == GET_100_MESSAGES_SUCCESS) {
                std::vector<database_interface::Message> messages_vector;
                for (const std::string &text_message: convert_to_text_vector_from_text(response.get_body())) {
                    database_interface::Message new_message;
                    Status current_status = database_interface::Message::parse_to_message(text_message, new_message);
                    if (!current_status) {
                        return {Status(false, "Can not convert message from:\t\t <" + text_message + ">"),
                                std::move(messages_vector)};
                    }
                    messages_vector.push_back(new_message);
                }
                return {Status(true), std::move(messages_vector)};
            } else {
                assert(response.get_type() == GET_100_MESSAGES_FAIL);
                if (connection_is_secured) {
                    response = decrypt_request(std::move(response));
                }
                return {Status(false, response.get_body()), {}};
            }
        }

        std::pair<Status, std::vector<database_interface::Dialog>> get_last_n_dialogs (int n_dialogs, int last_dialog_time = INT32_MAX) {
            std::vector<std::string> data_vector{std::to_string(n_dialogs),
                                                 std::to_string(last_dialog_time)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(GET_100_CHATS, data_to_send);
            } else {
                send_request(GET_100_CHATS, data_to_send);
            }

            Request response = get_request();
            if (connection_is_secured) {
                response = decrypt_request(std::move(response));
            }
            if (response && response.get_type() == GET_100_CHATS_SUCCESS) {
                std::vector<database_interface::Dialog> dialogs_vector;
                for (const std::string &text_dialog: convert_to_text_vector_from_text(response.get_body())) {
                    database_interface::Dialog new_dialog(-1);
                    Status current_status = database_interface::Dialog::parse_to_dialog(text_dialog, new_dialog);
                    if (!current_status) {
                        return {Status(false, "Can not convert status from:\t\t <" + text_dialog + ">"),
                                std::move(dialogs_vector)};
                    }
                    dialogs_vector.push_back(std::move(new_dialog));
                }
                return {Status(true), std::move(dialogs_vector)};
            } else {
                assert(response.get_type() == GET_100_CHATS_FAIL);
                return {Status(false, response.get_body()), {}};
            }
        }

        Status make_dialog(std::string dialog_name, std::string encryption, int current_time, int is_grope, const std::vector<unsigned int>& user_ids) {
            std::vector<std::string> data_vector{std::move(dialog_name), std::move(encryption), std::to_string(current_time), std::to_string(is_grope),
                                                 convert_int_vector_to_text(user_ids)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(MAKE_GROPE, data_to_send);
            } else {
                send_request(MAKE_GROPE, data_to_send);
            }
            Request response = get_request();
            response.parse_request();
            if (response && response.get_type() == MAKE_GROPE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == MAKE_GROPE_FAIL);
                if (connection_is_secured) {
                    response = decrypt_request(std::move(response));
                }
                return Status(false, response.get_body());
            }
        }

        Status log_in(std::string login, std::string password) {
            if (connection_is_secured) {
                send_secured_request(Net::LOG_IN_REQUEST, convert_text_vector_to_text({std::move(login), std::move(password)}));
            } else {
                send_request(Net::LOG_IN_REQUEST, convert_text_vector_to_text({std::move(login), std::move(password)}));
            }
            Request response = get_request();
            std::cout << "Response status: " + std::to_string(static_cast<bool>(response)) + "\n";
            if (connection_is_secured) {
                response = decrypt_request(std::move(response));
            }
            std::cout << "Response status: " + std::to_string(static_cast<int>(response.get_type())) + " with body: <" + response.get_body() + ">\n";
            std::cout << "Response status: " + std::to_string(static_cast<bool>(response)) + "\n";
            if (response.is_readable() && response.get_type() == LOG_IN_SUCCESS) {
                return Status(true, response.get_body());
            } else {
                if (response.get_type() != LOG_IN_FAIL) {
                    std::cerr << response.get_type() << "\n";
                    assert(response.get_type() == LOG_IN_FAIL);
                }
                if (connection_is_secured) {
                    response = decrypt_request(std::move(response));
                }
                return Status(false, response.get_body());
            }
        }

        Status sing_up(std::string name, std::string surname, std::string login, std::string password) {
            // Password is not really a password, but its hash.
            std::vector<std::string> data_vector{std::move(name), std::move(surname), std::move(login), std::move(password)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(SIGN_UP_REQUEST, data_to_send);
            } else {
                send_request(SIGN_UP_REQUEST, data_to_send);
            }
            Request response = get_request();
            if (connection_is_secured) {
                response = decrypt_request(std::move(response));
            }
//            if (response && response.get_type() == SIGN_UP_SUCCESS) {
            if (response.get_type() == SIGN_UP_SUCCESS) {
                return Status(true, response.get_body());
            } else {
                assert(response.get_type() == SIGN_UP_FAIL);
                return Status(false, response.get_body());
            }
        }

        Request get_request() {
            Request request = accept_request(connection.value(), connection_is_secured);
            if (connection_is_secured) {request.is_encrypted = true; }
            return std::move(request);
        }

        Request decrypt_request(Request request) { //NOLINT [static_method]
            if (!request.is_encrypted) {return std::move(request);}
            assert(connection_is_secured);
            auto decrypted_body = Cryptographer::as<std::string>(
                    decrypter.value().decrypt_data(request.get_encrypted_body()));
            request.set_body(decrypted_body);
            request.is_encrypted = false;
            return std::move(request);
        }

        void get_secret_request_and_out_it() {
            assert(connection_is_secured);
            Request request = accept_request(connection.value(), connection_is_secured);
            request = decrypt_request(std::move(request));
            std::cout << "Got from server: " << request.get_body() << "\n";
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
