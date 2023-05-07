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

#include "NetGeneral-Refactored.hpp"
#include "Cryptographer.hpp"
#include "./../../../include/database/Message.hpp"
#include "./../../../include/database/Dialog.hpp"
#include "../../../include/Status.hpp"


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

        void make_secure_connection() {
            // Establish connection
            boost::asio::ip::tcp::socket client_socket(io_context);
            boost::asio::connect(client_socket,
                                 boost::asio::ip::tcp::resolver(io_context).resolve(server_ip, server_port));
            connection = boost::asio::ip::tcp::iostream(std::move(client_socket));
            // Create decrypter and send public key
            decrypter = Cryptographer::Decrypter(Cryptographer::Cryptographer::get_rng());
            DecryptedRequest request1(MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY, json{{"public_key", decrypter.value().get_str_publicKey()}});
            send_request(request1.reinterpret_cast_to_encrypted());
            // Get other public key
            EncryptedRequest response1_not_casted(connection.value());
            DecryptedRequest response1 = response1_not_casted.reinterpret_cast_to_decrypted();
            assert(response1.request_type == MAKE_SECURE_CONNECTION_SUCCESS_RETURN_OTHER_KEY);
            encrypter = Cryptographer::Encrypter(response1.data["public_key"], Cryptographer::Cryptographer::get_rng());
            send_request(DecryptedRequest(MAKE_SECURE_CONNECTION_SUCCESS).reinterpret_cast_to_encrypted());
            connection_is_secured = true;
            std::cout << "Secured connection was established!\n";
        }

        void send_request(const EncryptedRequest& request) {
            assert(connection.has_value());
            Net::try_send_request(request, connection.value());
        }
        
        void get_request_and_out_it() {
            EncryptedRequest encrypted_request(connection.value());
            DecryptedRequest request = encrypted_request.decrypt(decrypter.value());
            auto true_string = request.data.dump(4);
            std::cout << "Got from server: " << true_string << "\n";
        }

        Status send_message_to_another_user(int dialog_id, int current_time, std::string text) {
            database_interface::Message new_message;
            new_message.m_dialog_id = dialog_id;
            new_message.m_date_time = current_time;
            new_message.m_text = std::move(text);
            
            DecryptedRequest request(SEND_MESSAGE, new_message);
            send_request(request.encrypt(encrypter.value()));

            DecryptedRequest response = get_request();
            if (response.get_type() == SEND_MESSAGE_SUCCESS) {
                return Status(true, response.data["message_id"]);
            } else {
                assert(response.get_type() == SEND_MESSAGE_FAIL);
                return Status(false, response.data["what"]);
            }
        }

        /*Status change_sent_message(int old_message_id, std::string new_text) {
            // We change text (message body) in old message, we get by id, to new_text.
            database_interface::Message message;
            message.m_message_id = old_message_id;
            message.m_text = std::move(new_text);
            send_request(DecryptedRequest(CHANGE_MESSAGE, message).encrypt(encrypter.value()));
            auto response = get_request();
            if (response.get_type() == CHANGE_MESSAGE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == CHANGE_MESSAGE_FAIL);
                return Status(false, response.data["what"]);
            }
        }*/

        Status change_sent_message(int old_message_id, std::string new_text) {
            // We change text (message body) in old message, we get by id, to new_text.
            database_interface::Message message;
            message.m_message_id = old_message_id;
            message.m_text = std::move(new_text);
            send_request(DecryptedRequest(CHANGE_MESSAGE, message).encrypt(encrypter.value()));
            auto response = get_request();
            if (response.get_type() == CHANGE_MESSAGE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == CHANGE_MESSAGE_FAIL);
                return Status(false, response.data["what"]);
            }
        }

        /*Status delete_message(int message_id) {
            std::vector<std::string> data_vector{std::to_string(message_id)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(DELETE_MESSAGE, data_to_send);
            } else {
                send_request(DELETE_MESSAGE, data_to_send);
            }
            Request response = get_request();
            if (connection_is_secured) {
                response = decrypt_request(std::move(response));
            }
            if (response.is_readable() && response.get_type() == DELETE_MESSAGE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == DELETE_MESSAGE_FAIL);
                return Status(false, response.data["what"]);
            }
        }*/

        Status delete_message(int message_id) {
            // We delete message by id.
            database_interface::Message message;
            message.m_message_id = message_id;
            send_request(DecryptedRequest(DELETE_MESSAGE, message).encrypt(encrypter.value()));
            auto response = get_request();
            if (response.get_type() == DELETE_MESSAGE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == DELETE_MESSAGE_FAIL);
                return Status(false, response.data["what"]);
            }
        }


        /*std::pair<Status, std::vector<database_interface::Message>>
        get_n_messages(int n, int dialog_id, int last_message_time = INT32_MAX) {
            std::vector<std::string> data_vector{std::to_string(n), std::to_string(dialog_id),
                                                 std::to_string(last_message_time)};
            std::string data_to_send = convert_text_vector_to_text(data_vector);
            if (connection_is_secured) {
                send_secured_request(GET_100_MESSAGES, data_to_send);
            } else {
                send_request(GET_100_MESSAGES, data_to_send);
            }
            Request response = get_request();
            if (connection_is_secured) {
                response = decrypt_request(std::move(response));
            }
            if (response.is_readable() && response.get_type() == GET_100_MESSAGES_SUCCESS) {
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
                return {Status(false, response.get_body()), {}};
            }
        }*/

        std::pair<Status, std::vector<database_interface::Message>>
        get_n_messages(int n, int dialog_id, int last_message_time = INT32_MAX) {
            database_interface::Dialog new_dialog;
            new_dialog.m_dialog_id = dialog_id;
            DecryptedRequest request(GET_100_MESSAGES, nlohmann::json{{"dialog", new_dialog},
                                                                      {"number_of_messages", n},
                                                                      {"last_message_time", last_message_time}});
            send_request(request.encrypt(encrypter.value()));
            DecryptedRequest response = get_request();
            if (response.get_type() == GET_100_MESSAGES_SUCCESS) {
                try {
                    std::vector<database_interface::Message> messages_vector = response.data["messages"];
                    return {Status(true), std::move(messages_vector)};
                } catch (std::exception &exception) {
                    return {Status(false, exception.what()), {}};
                }
            } else {
                assert(response.get_type() == GET_100_MESSAGES_FAIL);
                return {Status(false, response.data["what"]), {}};
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
            if (response.is_readable() && response.get_type() == GET_100_CHATS_SUCCESS) {
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
            if (response.is_readable() && response.get_type() == MAKE_GROPE_SUCCESS) {
                return Status(true);
            } else {
                assert(response.get_type() == MAKE_GROPE_FAIL);
                if (connection_is_secured) {
                    response = decrypt_request(std::move(response));
                }
                return Status(false, response.data["what"]);
            }
        }

        Status log_in(std::string login, std::string password) {
            assert(login.find_first_of("\t\n ") == std::string::npos);
            assert(password.find_first_of("\t\n ") == std::string::npos);
            if (connection_is_secured) {
                send_secured_request(Net::LOG_IN_REQUEST, convert_text_vector_to_text({std::move(login), std::move(password)}));
            } else {
                send_request(Net::LOG_IN_REQUEST, convert_text_vector_to_text({std::move(login), std::move(password)}));
            }
            Request response = get_request();
            if (connection_is_secured) {
                response = decrypt_request(std::move(response));
            }
            if (response.is_readable() && response.get_type() == LOG_IN_SUCCESS) {
                return Status(true, response.get_body());
            } else {
                if (response.get_type() != LOG_IN_FAIL) {
                    std::cerr << response.get_type() << "\n";
                    assert(response.get_type() == LOG_IN_FAIL);
                }
                return Status(false, response.data["what"]);
            }
        }

        Status close_connection() {
            send_request(DecryptedRequest(Net::LOG_IN_REQUEST).encrypt(encrypter.value()));
            connection->close();
            return Status{true};
        }

        Status sing_up(std::string name, std::string surname, std::string login, std::string password) {
            assert(login.find_first_of("\t\n ") == std::string::npos);
            assert(password.find_first_of("\t\n ") == std::string::npos);
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
            if (response.is_readable() && response.get_type() == SIGN_UP_SUCCESS) {
                return Status(true, response.get_body());
            } else {
                assert(response.get_type() == SIGN_UP_FAIL);
                return Status(false, response.data["what"]);
            }
        }

        std::pair<Status, database_interface::User> get_user_id_by_login(const std::string& login) {
            assert(login.find_first_of("\t\n ") == std::string::npos);
            if (connection_is_secured) {
                send_secured_request(GET_USER_BY_LOGIN, login);
            } else {
                send_request(GET_USER_BY_LOGIN, login);
            }

            Request response = get_request();
            if (connection_is_secured) {
                response = decrypt_request(std::move(response));
            }
            if (response.is_readable() && response.get_type() == GET_USER_BY_LOGIN_SUCCESS) {
                database_interface::User user{};
                std::vector<std::string> user_data = convert_to_text_vector_from_text(response.get_body());
                assert(user_data.size() == 4);
                assert(is_number(user_data[0]));
                user.m_user_id = std::stoi(user_data[0]);
                user.m_name = std::move(user_data[1]);
                user.m_surname = std::move(user_data[2]);;
                user.m_login = std::move(user_data[3]);;
                return {Status(true, response.get_body()), std::move(user)};
            } else {
                assert(response.get_type() == SIGN_UP_FAIL);
                return {Status(false, response.get_body()), database_interface::User{}};
            }
        };

        DecryptedRequest get_request() {
            EncryptedRequest request(connection.value());
            return std::move(request.decrypt(decrypter.value()));
        }

        std::string get_request_and_return_text_body() {
            auto request = get_request();
            return "Got from server: " + request.data.dump(4);
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
