//
// Created by andrey on 15.03.23.
//

#ifndef MESSENGER_PROJECT_NETCLIENT_HPP
#define MESSENGER_PROJECT_NETCLIENT_HPP

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Cryptographer.hpp"
#include "FileWorker.hpp"
#include "Net/NetGeneral.hpp"
#include "Status.hpp"
#include "database/Dialog.hpp"
#include "database/Message.hpp"

namespace Net::Client {

struct Client {
public:
#ifndef MULTI_CLIENT_TEST

    explicit Client(
        std::string server_ip_ = "localhost",
        std::string port_ = "12345"
    )
        : server_ip(std::move(server_ip_)), server_port(std::move(port_)){};
#else
    Client(
        boost::asio::io_context &ioContext,
        std::string server_ip_ = "localhost",
        std::string port_ = "12345"
    )
        : io_context(ioContext),
          server_ip(std::move(server_ip_)),
          server_port(std::move(port_)){};
#endif

    void make_secure_connection(std::string encryption_name = "RSA") {
        // Establish connection
        boost::asio::ip::tcp::socket client_socket(io_context);
        boost::asio::connect(
            client_socket, boost::asio::ip::tcp::resolver(io_context)
                               .resolve(server_ip, server_port)
        );
        connection = boost::asio::ip::tcp::iostream(std::move(client_socket));
        // Create decrypter and send public key
        decrypter = Cryptographer::Decrypter(
            Cryptographer::Cryptographer::get_rng(), encryption_name
        );
        DecryptedRequest request1(
            MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY,
            json{
                {"public_key", decrypter.value().get_str_publicKey()},
                {"encryption_name", encryption_name}}
        );
        send_request(request1.reinterpret_cast_to_encrypted());
        // Get other public key
        EncryptedRequest response1_not_casted(connection.value());
        DecryptedRequest response1 =
            response1_not_casted.reinterpret_cast_to_decrypted();
        assert(
            response1.request_type ==
            MAKE_SECURE_CONNECTION_SUCCESS_RETURN_OTHER_KEY
        );
        encrypter = Cryptographer::Encrypter(
            response1.data["public_key"],
            Cryptographer::Cryptographer::get_rng()
        );
        send_request(DecryptedRequest(MAKE_SECURE_CONNECTION_SUCCESS)
                         .reinterpret_cast_to_encrypted());
        connection_is_secured = true;
        std::cout << "Secured connection was established!\n";
    }

    void send_request(const EncryptedRequest &request) {
        assert(connection.has_value());
        Net::try_send_request(request, connection.value());
    }

    void get_request_and_out_it() {
        EncryptedRequest encrypted_request(connection.value());
        DecryptedRequest request = encrypted_request.decrypt(decrypter.value());
        auto true_string = request.data.dump(4);
        std::cout << "Got from server: " << true_string << "\n";
    }

    Status send_message_to_another_user(
        int dialog_id,
        std::string file_name,
        std::string text
    ) {
        database_interface::Message new_message;
        new_message.m_dialog_id = dialog_id;
        new_message.m_file_name = std::move(file_name);
        new_message.m_text = std::move(text);

        DecryptedRequest request(SEND_MESSAGE, new_message);
        send_request(request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == SEND_MESSAGE_SUCCESS) {
            return Status(true, response.data.dump(4));
        } else {
            assert(response.get_type() == SEND_MESSAGE_FAIL);
            return Status(false, response.data["what"]);
        }
    }

    /*Status change_sent_message(int old_message_id, std::string new_text) {
        // We change text (message body) in old message, we get by id, to
    new_text. database_interface::Message message; message.m_message_id =
    old_message_id; message.m_text = std::move(new_text);
        send_request(DecryptedRequest(CHANGE_MESSAGE,
    message).encrypt(encrypter.value())); auto response = get_request(); if
    (response.get_type() == CHANGE_MESSAGE_SUCCESS) { return Status(true); }
    else { assert(response.get_type() == CHANGE_MESSAGE_FAIL); return
    Status(false, response.data["what"]);
        }
    }*/

    Status change_sent_message(int old_message_id, std::string new_text) {
        // We change text (message body) in old message, we get by id, to
        // new_text.
        database_interface::Message message;
        message.m_message_id = old_message_id;
        message.m_text = std::move(new_text);
        send_request(
            DecryptedRequest(CHANGE_MESSAGE, message).encrypt(encrypter.value())
        );
        auto response = get_request();
        if (response.get_type() == CHANGE_MESSAGE_SUCCESS) {
            return Status(true);
        } else {
            assert(response.get_type() == CHANGE_MESSAGE_FAIL);
            return Status(false, response.data["what"]);
        }
    }

    Status delete_message(int message_id) {
        // We delete message by id.
        database_interface::Message message;
        message.m_message_id = message_id;
        send_request(
            DecryptedRequest(DELETE_MESSAGE, message).encrypt(encrypter.value())
        );
        auto response = get_request();
        if (response.get_type() == DELETE_MESSAGE_SUCCESS) {
            return Status(true);
        } else {
            assert(response.get_type() == DELETE_MESSAGE_FAIL);
            return Status(false, response.data["what"]);
        }
    }

    std::pair<Status, std::vector<database_interface::Message>>
    get_n_messages(int n, int dialog_id, int last_message_time = INT32_MAX) {
        database_interface::Dialog new_dialog;
        new_dialog.m_dialog_id = dialog_id;
        DecryptedRequest request(
            GET_100_MESSAGES,
            nlohmann::json{
                {"dialog", new_dialog},
                {"number_of_messages", n},
                {"last_message_time", last_message_time}}
        );
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        if (response.get_type() == GET_100_MESSAGES_SUCCESS) {
            try {
                std::vector<database_interface::Message> messages_vector =
                    response.data["messages"];
                return {Status(true), std::move(messages_vector)};
            } catch (std::exception &exception) {
                return {Status(false, exception.what()), {}};
            }
        } else {
            std::cout << "get type" << response.get_type() << "\n";
            assert(response.get_type() == GET_100_MESSAGES_FAIL);
            return {Status(false, response.data["what"]), {}};
        }
    }

    std::pair<Status, std::vector<database_interface::Dialog>>
    get_last_n_dialogs(int n_dialogs, int last_dialog_time = INT32_MAX) {
        DecryptedRequest decrypted_request(
            GET_100_CHATS,
            {{"n_dialogs", n_dialogs}, {"last_dialog_time", last_dialog_time}}
        );
        send_request(decrypted_request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == GET_100_CHATS_SUCCESS) {
            std::vector<database_interface::Dialog> dialogs_vector =
                response.data["dialogs"];
            return {Status(true), std::move(dialogs_vector)};
        } else {
            assert(response.get_type() == GET_100_CHATS_FAIL);
            return {Status(false, response.data["what"]), {}};
        }
    }

    Status make_dialog(
        std::string dialog_name,
        int current_time,
        int is_group,
        const std::vector<unsigned int> &user_ids
    ) {
        database_interface::Dialog new_dialog;
        nlohmann::json dialog_json;

        new_dialog.m_name = std::move(dialog_name);
        new_dialog.m_date_time = current_time;
        new_dialog.m_is_group = is_group;

        dialog_json["user_ids"] = user_ids;
        dialog_json["dialog"] = new_dialog;

        DecryptedRequest request(MAKE_GROPE, dialog_json);
        send_request(request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == MAKE_GROPE_SUCCESS) {
            return Status(true);
        } else {
            assert(response.get_type() == MAKE_GROPE_FAIL);
            return Status(false, response.data["what"]);
        }
    }

    Status upload_file(const FileWorker::File &file) {
        DecryptedRequest request(FILE_UPLOAD, file.operator nlohmann::json());
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        return Status(
            response.get_type() == FILE_UPLOAD_SUCCESS,
            response.get_type() == FILE_UPLOAD_SUCCESS
                ? response.data["file_name"]
                : response.data["what"]
        );
    }

    std::pair<Status, FileWorker::File> download_file(
        const std::string &file_name
    ) {
        DecryptedRequest request(FILE_DOWNLOAD, json{{"file_name", file_name}});
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        if (response.get_type() == FILE_DOWNLOAD_SUCCESS) {
            std::optional<FileWorker::File> file;
            try {
                file = FileWorker::File(response.data, FileWorker::parse_JSON);
            } catch (FileWorker::file_exception &exception) {
                return {
                    Status(
                        false,
                        "Cannot parse file: " + std::string(exception.what())
                    ),
                    FileWorker::File(FileWorker::empty_file)};
            }
            return {Status(true, ""), *file};
        } else {
            if (response.get_type() != FILE_DOWNLOAD_FAIL) {
                std::cerr << "Expected FILE_DOWNLOAD_FAIL with json problem "
                             "message, but got instead:"
                          << std::endl;
                std::cerr << response.get_type() << std::endl;
                std::cerr << response.data.dump(4) << std::endl;
                assert(response.get_type() == FILE_DOWNLOAD_FAIL);
            }
            return {
                Status(false, response.data["what"]),
                FileWorker::File(FileWorker::empty_file)};
        }
    }

    std::pair<Status, std::vector<database_interface::User>>
    get_users_in_dialog(int dialog_id) {
        DecryptedRequest request(
            GET_USERS_IN_DIALOG, json{{"dialog_id", dialog_id}}
        );
        send_request(request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == GET_USERS_IN_DIALOG_SUCCESS) {
            std::vector<database_interface::User> users;
            try {
                users = response.data;
            } catch (std::exception &exception) {
                return {
                    Status(
                        false,
                        std::string(
                            "Parse get_users_in_dialog response exception: "
                        ) + exception.what()
                    ),
                    {}};
            }
            return {Status(true, ""), users};
        } else {
            assert(response.get_type() == GET_USERS_IN_DIALOG_FAIL);
            return {Status(false, response.data["what"]), {}};
        }
    }

    Status add_user_to_dialog(int user_id, int dialog_id) {
        DecryptedRequest request(
            ADD_USER_TO_DIALOG,
            json{{"user_id", user_id}, {"dialog_id", dialog_id}}
        );
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        if (response.get_type() == ADD_USER_TO_DIALOG_SUCCESS) {
            return Status(true, "");
        } else {
            assert(response.get_type() == ADD_USER_TO_DIALOG_FAIL);
            return Status(false, response.data["what"]);
        }
    }

    Status del_user_from_dialog(int user_id, int dialog_id) {
        DecryptedRequest request(
            DEL_USER_FROM_DIALOG,
            json{{"user_id", user_id}, {"dialog_id", dialog_id}}
        );
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        if (response.get_type() == DEL_USER_FROM_DIALOG_SUCCESS) {
            return Status(true, "");
        } else {
            assert(response.get_type() == DEL_USER_FROM_DIALOG_FAIL);
            return Status(false, response.data["what"]);
        }
    }

    std::pair<Status, database_interface::Dialog> get_dialog_by_id(int dialog_id
    ) {
        DecryptedRequest request(
            GET_DIALOG_BY_ID, json{{"dialog_id", dialog_id}}
        );
        send_request(request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == GET_DIALOG_BY_ID_SUCCESS) {
            database_interface::Dialog dialog = response.data;
            return {Status(true, ""), std::move(dialog)};
        } else {
            std::cout << response.get_type() << "\n";
            assert(response.get_type() == GET_DIALOG_BY_ID_FAIL);
            return {
                Status(false, response.data["what"]),
                database_interface::Dialog{}};
        }
    };

    std::pair<Status, database_interface::User>
    log_in(std::string login, std::string password) {
        assert(login.find_first_of("\t\n ") == std::string::npos);
        assert(password.find_first_of("\t\n ") == std::string::npos);
        database_interface::User user(std::move(login), std::move(password));
        DecryptedRequest request(LOG_IN_REQUEST, user);
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        if (response.get_type() == LOG_IN_SUCCESS) {
            user = response.data;
            auto exit = get_encryption_by_id(user.m_encryption);
            if (exit.first) {
                close_connection();
                make_secure_connection(exit.second);
                send_request(request.encrypt(encrypter.value()));
                response = get_request();
                assert(response.get_type() == LOG_IN_SUCCESS);
                return {Status(true, ""), std::move(user)};
            }
        }
        assert(response.get_type() == LOG_IN_FAIL);
        return {
            Status(false, response.data["what"]), database_interface::User{}};
    }

    std::pair<Status, std::string> get_encryption_by_id(int encryption_id) {
        DecryptedRequest request(
            GET_ENCRYPTION, json{{"encryption_id", encryption_id}}
        );
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        if (response.get_type() == GET_ENCRYPTION_SUCCESS) {
            std::string encryption;
            try {
                encryption = response.data;
            } catch (std::exception &exception) {
                return {
                    Status(
                        false,
                        std::string(
                            "Parse get_all_encryption response exception: "
                        ) + exception.what()
                    ),
                    {}};
            }
            return {Status(true, ""), encryption};
        } else {
            assert(response.get_type() == GET_ENCRYPTION_FAIL);
            return {Status(false, response.data["what"]), {}};
        }
    }

    std::pair<Status, std::vector<std::pair<int, std::string>>>
    get_all_encryption() {
        DecryptedRequest request(GET_ALL_ENCRYPTION);
        send_request(request.encrypt(encrypter.value()));
        DecryptedRequest response = get_request();
        if (response.get_type() == GET_ALL_ENCRYPTION_SUCCESS) {
            std::vector<std::pair<int, std::string>> encryption;
            try {
                encryption = response.data;
            } catch (std::exception &exception) {
                return {
                    Status(
                        false,
                        std::string(
                            "Parse get_all_encryption response exception: "
                        ) + exception.what()
                    ),
                    {}};
            }
            return {Status(true, ""), encryption};
        } else {
            assert(response.get_type() == GET_ALL_ENCRYPTION_FAIL);
            return {Status(false, response.data["what"]), {}};
        }
    }

    Status close_connection() {
        send_request(
            DecryptedRequest(Net::CLOSE_CONNECTION).encrypt(encrypter.value())
        );
        connection->close();
        return Status{true};
    }

    Status sign_up(
        std::string name,
        std::string surname,
        std::string login,
        std::string password
    ) {
        assert(login.find_first_of("\t\n ") == std::string::npos);
        assert(password.find_first_of("\t\n ") == std::string::npos);
        // Password is not really a password, but its hash.
        database_interface::User user;
        user.m_name = std::move(name);
        user.m_surname = std::move(surname);
        user.m_login = std::move(login);
        user.m_password_hash = std::move(password);

        DecryptedRequest request(SIGN_UP_REQUEST, user);
        send_request(request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == SIGN_UP_SUCCESS) {
            user = response.data;
            return Status(true, std::to_string(user.m_user_id));
        }
        assert(response.get_type() == SIGN_UP_FAIL);
        return Status(false, response.data["what"]);
    }

    std::pair<Status, database_interface::User> change_user(
        int old_id,
        std::string changing_param,
        std::string new_param,
        int new_encrypt
    ) {
        assert(new_param.find_first_of("\t\n ") == std::string::npos);
        assert(
            changing_param == "name" || changing_param == "surname" ||
            changing_param == "login" || changing_param == "password" ||
            changing_param == "encryption"
        );
        if (changing_param == "encryption") {
            DecryptedRequest request(
                CHANGE_USER,
                json{
                    {"old_id", old_id},
                    {"changing_in", std::move(changing_param)},
                    {"new_parametr", new_encrypt}}
            );
            send_request(request.encrypt(encrypter.value()));
        } else {
            DecryptedRequest request(
                CHANGE_USER,
                json{
                    {"old_id", old_id},
                    {"changing_in", std::move(changing_param)},
                    {"new_parametr", std::move(new_param)}}
            );
            send_request(request.encrypt(encrypter.value()));
        }

        auto response = get_request();
        if (response.get_type() == CHANGE_USER_SUCCESS) {
            database_interface::User user = response.data;
            return {Status(true), user};
        } else {
            assert(response.get_type() == CHANGE_USER_FAIL);
            return {
                Status(false, response.data["what"]),
                database_interface::User{}};
        }
    }

    std::pair<Status, database_interface::User> get_user_id_by_login(
        const std::string &login
    ) {
        assert(login.find_first_of("\t\n ") == std::string::npos);
        DecryptedRequest request(GET_USER_BY_LOGIN, json{{"login", login}});
        send_request(request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == GET_USER_BY_LOGIN_SUCCESS) {
            database_interface::User user = response.data;
            return {Status(true, ""), std::move(user)};
        } else {
            assert(response.get_type() == GET_USER_BY_LOGIN_FAIL);
            return {
                Status(false, response.data["what"]),
                database_interface::User{}};
        }
    };

    std::pair<Status, database_interface::User> get_user_by_id(int id) {
        DecryptedRequest request(GET_USER_BY_ID, json{{"user_id", id}});
        send_request(request.encrypt(encrypter.value()));

        DecryptedRequest response = get_request();
        if (response.get_type() == GET_USER_BY_ID_SUCCESS) {
            database_interface::User user = response.data;
            return {Status(true, ""), std::move(user)};
        } else {
            assert(response.get_type() == GET_USER_BY_ID_FAIL);
            return {
                Status(false, response.data["what"]),
                database_interface::User{}};
        }
    };

    std::string send_text_request_and_return_response_text(
        const std::string &message
    ) {
        send_request(Net::DecryptedRequest(
                         Net::SECURED_REQUEST, json{{"text", message}}
        ).encrypt(encrypter.value()));
        auto response = get_request();
        assert(response.get_type() == RESPONSE_REQUEST_SUCCESS);
        return response.data["text"];
    }

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

}  // namespace Net::Client

#endif  // MESSENGER_PROJECT_NETCLIENT_HPP
