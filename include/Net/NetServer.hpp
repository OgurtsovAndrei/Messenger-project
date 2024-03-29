//
// Created by andrey on 08.03.23.
//

#ifndef MESSENGER_PROJECT_NETSERVER_HPP
#define MESSENGER_PROJECT_NETSERVER_HPP

#include <boost/asio.hpp>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
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
#include "TextWorker.hpp"
#include "database/DataBaseInterface.hpp"
#include "database/User.hpp"

#define send_response_and_return_if_false(                     \
    expr, user, assert_request_type, message                   \
)                                                              \
    if (!(expr)) {                                             \
        user.send_secured_request(DecryptedRequest(            \
            assert_request_type,                               \
            to_json_exception(                                 \
                "Fail in assert(" #expr "), with message: <" + \
                static_cast<std::string>(message) + ">"        \
            )                                                  \
        ));                                                    \
        return;                                                \
    }                                                          \
    while (0)

json to_json_exception(std::string str) {
    json j;
    j["what"] = std::move(str);
    return j;
}

#define require(expr, message)         \
    if (!(expr)) {                     \
        return Status(false, message); \
    }                                  \
    while (0)

namespace Net::Server {

struct Server;

struct UserConnection {
public:
    explicit UserConnection(
        boost::asio::ip::tcp::socket &&socket_,
        Server &server_,
        int connection_number_
    )
        : server(server_),
          connection_number(connection_number_),
          current_socket(std::move(socket_)){};

    UserConnection(const UserConnection &con) = delete;

    UserConnection(UserConnection &&) = default;

    UserConnection &operator=(const UserConnection &) = delete;

    UserConnection &operator=(UserConnection &&) = delete;

    void work_with_connection(
        boost::asio::ip::tcp::socket &&socket,
        UserConnection &connection
    );

    void run_connection() {
        session_thread =
            std::move(std::thread([&, socket = std::move(current_socket),
                                   this]() mutable {
                work_with_connection(std::move(socket), *this);
            }));
    }

    [[nodiscard]] bool is_protected() const { return connection_is_protected; }

    std::optional<database_interface::User> &get_user_in_db_ref() {
        return user_in_db;
    }

    Status send_secured_request(const DecryptedRequest &decrypted_request) {
        require(connection_is_protected, "Connection should be secured!!");
        require(encrypter.has_value(), "Connection should be secured!!");
        EncryptedRequest encrypted_request =
            decrypted_request.encrypt(encrypter.value());
        std::unique_lock lock(*client_connection_out_mutex);
        Net::try_send_request(encrypted_request, client.value());
        return Status(true);
    }

    Status
    send_secured_exception(RequestType type, std::string exception_message) {
        require(connection_is_protected, "Connection should be secured!!");
        require(encrypter.has_value(), "Connection should be secured!!");

        EncryptedRequest encrypted_request =
            DecryptedRequest(
                type, to_json_exception(std::move(exception_message))
            )
                .encrypt(encrypter.value());
        std::unique_lock lock(*client_connection_out_mutex);
        Net::try_send_request(encrypted_request, client.value());
        return Status(true);
    }

    ~UserConnection() {
        if (session_thread) {
            //  session_thread.value().join();
            session_thread->detach();
        }
    }

private:
    friend struct Server;
    // Must be initialized
    int connection_number;
    boost::asio::ip::tcp::socket current_socket;
    Server &server;
    std::unique_ptr<std::mutex> client_connection_out_mutex =
        std::make_unique<std::mutex>();
    std::unique_ptr<std::mutex> client_connection_in_mutex =
        std::make_unique<std::mutex>();
    // Not necessary to init
    std::vector<DecryptedRequest> connection_requests;
    std::optional<boost::asio::ip::tcp::iostream> client;
    std::optional<std::thread> session_thread;
    bool connection_is_protected = false;
    bool connection_is_active = false;
    Cryptographer::Cryptographer cryptographer;
    std::optional<Cryptographer::Encrypter> encrypter;
    std::optional<Cryptographer::Decrypter> decrypter;
    std::optional<database_interface::User> user_in_db;

    static void accept_client_request(
        const std::string &rem_endpoint_str,
        UserConnection &connection
    );
};

struct RequestQueue {
public:
    RequestQueue() = default;

    [[nodiscard]] std::pair<int, DecryptedRequest> get_last_or_wait() {
        std::unique_lock queue_lock(queue_mutex);
        while (request_queue.empty()) {
            cond_var.wait(queue_lock);
        }
        auto value = std::move(request_queue.front());
        request_queue.pop();
        return std::move(value);
    }

    void push_to_queue(int number, DecryptedRequest value) {
        std::unique_lock queue_lock(queue_mutex);
        request_queue.emplace(number, std::move(value));
        cond_var.notify_one();
    }

    void push_to_queue(std::pair<int, DecryptedRequest> value) {
        std::unique_lock queue_lock(queue_mutex);
        request_queue.push(std::move(value));
        cond_var.notify_one();
    }

private:
    std::mutex queue_mutex;
    std::queue<std::pair<int, DecryptedRequest>> request_queue;
    std::condition_variable cond_var;
};

struct Server {
public:
    explicit Server(const int port_ = 80)
        : port(port_),
          connection_acceptor(
              io_context,
              boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_)
          ) {
        std::cout << "Listening at " << connection_acceptor.local_endpoint()
                  << "\n";
    }

    [[nodiscard]] [[maybe_unused]] int get_port() const { return port; }

    [[noreturn]] void run_server(int number_of_thread_in_pool = 1) {
        for (int consumer_id = 0; consumer_id < number_of_thread_in_pool;
             ++consumer_id) {
            consumers.push_back(std::move(std::thread([&, this]() mutable {
                while (true) {  // NOLINT
                    // В очереди хранятся уже расшифрованные request-ы
                    auto [connection_id, request] =
                        request_queue.get_last_or_wait();
                    std::unique_lock sessions_lock(sessions_mutex);
                    auto iter = sessions.find(connection_id);
                    if (iter == sessions.end()) {
                        continue;
                    }
                    UserConnection &user_connection = iter->second;
                    sessions_lock.unlock();
                    std::cout << "Got request from user with --> "
                              << connection_id << " connection id\n";
                    switch (request.request_type) {
                        case TEXT_REQUEST:
                            [[fallthrough]];
                        case SECURED_REQUEST: {
                            json data;
                            data["text"] =
                                "Got from you: <" +
                                static_cast<std::string>(request.data["text"]) +
                                ">";
                            user_connection.send_secured_request(
                                DecryptedRequest(
                                    RESPONSE_REQUEST_SUCCESS, std::move(data)
                                )
                            );
                        } break;
                        case RESPONSE_REQUEST_SUCCESS:
                            break;
                        case RESPONSE_REQUEST_FAIL:
                            break;
                        case MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY:
                            break;
                        case MAKE_SECURE_CONNECTION_SUCCESS_RETURN_OTHER_KEY:
                            break;
                        case MAKE_SECURE_CONNECTION_SUCCESS:
                            break;
                        case MAKE_SECURE_CONNECTION_FAIL:
                            break;
                        case LOG_IN_REQUEST:
                            break;
                        case LOG_IN_SUCCESS:
                            break;
                        case LOG_IN_FAIL:
                            break;
                        case GET_USER_BY_LOGIN:
                            process_get_user_by_login_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case GET_100_CHATS:
                            process_get_n_dialogs_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case SEND_DIALOG_REQUEST:
                            break;
                        case GET_ALL_DIALOG_REQUESTS:
                            break;
                        case DENY_DIALOG_REQUEST:
                            break;
                        case ACCEPT_DIALOG_REQUEST:
                            break;
                        case MAKE_GROPE:
                            process_make_grope_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case DELETE_DIALOG:
                            break;
                        case SEND_MESSAGE:
                            process_send_message_request(
                                user_connection, request
                            );
                            break;
                        case CHANGE_MESSAGE:
                            process_change_old_message_request(
                                user_connection, request
                            );
                            break;
                        case DELETE_MESSAGE:
                            proces_delete_message_request(
                                user_connection, request
                            );
                            break;
                        case GET_100_MESSAGES:
                            process_get_n_messages_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case SIGN_UP_REQUEST:
                            break;
                        case SIGN_UP_SUCCESS:
                            break;
                        case CLOSE_CONNECTION:
                            user_connection.connection_is_active = false;
                            break;
                        case UNKNOWN:
                            break;
                        case GET_100_CHATS_SUCCESS:
                            break;
                        case GET_100_CHATS_FAIL:
                            break;
                        case MAKE_GROPE_SUCCESS:
                            break;
                        case MAKE_GROPE_FAIL:
                            break;
                        case SEND_MESSAGE_SUCCESS:
                            break;
                        case SEND_MESSAGE_FAIL:
                            break;
                        case CHANGE_MESSAGE_SUCCESS:
                            break;
                        case CHANGE_MESSAGE_FAIL:
                            break;
                        case DELETE_MESSAGE_SUCCESS:
                            break;
                        case DELETE_MESSAGE_FAIL:
                            break;
                        case GET_100_MESSAGES_SUCCESS:
                            break;
                        case GET_100_MESSAGES_FAIL:
                            break;
                        case GET_USER_BY_LOGIN_SUCCESS:
                            break;
                        case GET_USER_BY_LOGIN_FAIL:
                            break;
                        case SIGN_UP_FAIL:
                            break;
                        case ADD_USER_TO_DIALOG:
                            process_add_user_to_dialog_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case ADD_USER_TO_DIALOG_SUCCESS:
                            break;
                        case ADD_USER_TO_DIALOG_FAIL:
                            break;
                        case DEL_USER_FROM_DIALOG:
                            process_del_user_from_dialog_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case DEL_USER_FROM_DIALOG_SUCCESS:
                            break;
                        case DEL_USER_FROM_DIALOG_FAIL:
                            break;
                        case GET_DIALOG_BY_ID:
                            process_get_dialog_by_id_request(
                                user_connection, std::move(request)
                            );
                        case GET_DIALOG_BY_ID_SUCCESS:
                            break;
                        case GET_DIALOG_BY_ID_FAIL:
                            break;
                        case GET_USER_BY_ID:
                            process_get_user_by_id_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case GET_USER_BY_ID_SUCCESS:
                            break;
                        case GET_USER_BY_ID_FAIL:
                            break;
                        case GET_ENCRYPTION:
                            process_get_encryption_by_id_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case GET_ENCRYPTION_SUCCESS:
                            break;
                        case GET_ENCRYPTION_FAIL:
                            break;
                        case GET_ALL_ENCRYPTION:
                            process_get_all_encryption_request(
                                user_connection, std::move(request)
                            );
                            break;
                        case GET_ALL_ENCRYPTION_SUCCESS:
                            break;
                        case GET_ALL_ENCRYPTION_FAIL:
                            break;
                        case FILE_DOWNLOAD:
                            process_download_file_request(
                                user_connection, request
                            );
                            break;
                        case FILE_DOWNLOAD_SUCCESS:
                            break;
                        case FILE_DOWNLOAD_FAIL:
                            break;
                        case FILE_UPLOAD:
                            process_upload_file_request(
                                user_connection, request
                            );
                            break;
                        case FILE_UPLOAD_SUCCESS:
                            break;
                        case FILE_UPLOAD_FAIL:
                            break;
                        case GET_USERS_IN_DIALOG:
                            process_get_users_in_dialog_request(
                                user_connection, request
                            );
                            break;
                        case GET_USERS_IN_DIALOG_SUCCESS:
                            break;
                        case GET_USERS_IN_DIALOG_FAIL:
                            break;
                        case CHANGE_USER:
                            process_change_user_request(
                                user_connection, request
                            );
                            break;
                        case CHANGE_USER_SUCCESS:
                            break;
                        case CHANGE_USER_FAIL:
                            break;
                    }
                }
            })));
        }
        while (true) {
            make_connection();
        }
    }

    void run_server_thread() {
        server_thread = std::move(std::thread([&, this]() mutable {
            while (true) {
                make_connection();
            }
        }));
    }

    void make_connection() {
        boost::asio::ip::tcp::socket socket = connection_acceptor.accept();
        Server &server_ref = *this;
        int empty_number = find_empty_connection_number();
        sessions.emplace(
            empty_number,
            UserConnection(std::move(socket), server_ref, empty_number)
        );
        UserConnection &connection = sessions.find(empty_number)->second;
        connection.run_connection();
    }

    void close_connection(int connection_number) {
        assert(sessions.count(connection_number));
        sessions.erase(connection_number);
    }

    Status open_database() { return bd_connection.open(); }

    Status close_database() { return bd_connection.close(); }

    void push_request_to_queue(int number, DecryptedRequest value) {
        request_queue.push_to_queue(number, std::move(value));
    }

    void push_request_to_queue(std::pair<int, DecryptedRequest> value) {
        request_queue.push_to_queue(std::move(value));
    }

    database_interface::SQL_BDInterface &get_bd_connection_ref() {
        return bd_connection;
    }

    void
    sign_up(UserConnection &user_connection, const DecryptedRequest &request) {
        database_interface::User new_user;
        try {
            nlohmann::from_json(request.data, new_user);
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                SIGN_UP_FAIL,
                "Can not parse request: bad request body format or invalid "
                "request type: " +
                    static_cast<std::string>(exception.what())
            );
        }
        send_response_and_return_if_false(
            new_user.m_login.find_first_of("\t\n /") == std::string::npos,
            user_connection, SIGN_UP_FAIL, "Login should contain only one word!"
        );

        send_response_and_return_if_false(
            new_user.m_password_hash.find_first_of("\t\n ") ==
                std::string::npos,
            user_connection, SIGN_UP_FAIL,
            "Password should contain only one word!"
        );
        auto status = bd_connection.make_user(new_user);
        if (status) {
            database_interface::User user_copy = new_user;
            user_copy.m_password_hash.clear();
            user_connection.send_secured_request(
                DecryptedRequest(SIGN_UP_SUCCESS, user_copy)
            );
        } else {
            user_connection.send_secured_exception(
                SIGN_UP_FAIL, status.message()
            );
        }
    }

    void log_in(
        UserConnection &user_connection,
        DecryptedRequest &request,
        bool &logging_in_is_successful
    ) {
        user_connection.user_in_db = database_interface::User();
        database_interface::User &user_in_db =
            user_connection.user_in_db.value();
        try {
            nlohmann::from_json(request.data, user_in_db);
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                LOG_IN_FAIL,
                "Not able to logg in: cannot parse json user: bad request or "
                "invalid user data: " +
                    static_cast<std::string>(exception.what())
            );
        }

        send_response_and_return_if_false(
            user_in_db.m_login.find_first_of("\t\n ") == std::string::npos,
            user_connection, SIGN_UP_FAIL, "Login should contain only one word!"
        );

        send_response_and_return_if_false(
            user_in_db.m_password_hash.find_first_of("\t\n ") ==
                std::string::npos,
            user_connection, SIGN_UP_FAIL,
            "Password should contain only one word!"
        );
        auto status = bd_connection.get_user_by_log_pas(user_in_db);
        if (status) {
            database_interface::User user_copy = user_in_db;
            user_copy.m_password_hash.clear();
            user_connection.send_secured_request(
                DecryptedRequest(LOG_IN_SUCCESS, user_copy)
            );
            logging_in_is_successful = true;
            std::cout << "User with id: " +
                             std::to_string(user_in_db.m_user_id) +
                             " logged in!\n";
        } else {
            user_connection.send_secured_exception(
                LOG_IN_FAIL, status.message()
            );
        }
    }

private:
    std::optional<std::thread> server_thread;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor connection_acceptor;
    const int port;
    std::mutex sessions_mutex;
    std::unordered_map<int, UserConnection> sessions;
    RequestQueue request_queue;
    std::vector<std::thread> consumers;
    database_interface::SQL_BDInterface bd_connection;

    void process_send_message_request(
        UserConnection &user_connection,
        const DecryptedRequest &request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            SEND_MESSAGE_FAIL, "It is necessary to log in!"
        );
        database_interface::Message new_message;
        try {
            nlohmann::from_json(request.data, new_message);
            new_message.m_user_id = user_connection.user_in_db->m_user_id;
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                SEND_MESSAGE_FAIL,
                "Not able to send message: cannot parse json message: bad "
                "request or invalid message data: " +
                    static_cast<std::string>(exception.what())
            );
        }

        if (!new_message.m_file_name.empty()) {
            //                send_file(user_connection, new_message);
            std::cout << "try to send message"
                      << "\n";
            std::vector<database_interface::User> users;
            FileWorker::File file(FileWorker::empty_file);
            std::cout << "here-" << 1 << "\n";
            std::string file_path = "./../bd/Files/users_files/" +
                                    user_connection.user_in_db->m_login + "/" +
                                    new_message.m_file_name;
            try {
                file = FileWorker::File(file_path);
            } catch (FileWorker::file_exception &exception) {
                send_response_and_return_if_false(
                    false, user_connection, SEND_MESSAGE_FAIL, exception.what()
                );
            }
            std::cout << "here-" << 2 << "\n";
            send_response_and_return_if_false(
                std::filesystem::exists(file_path), user_connection,
                SEND_MESSAGE_FAIL, "This file does not exist!"
            );

            std::cout << "here-" << 3 << "\n";
            Status users_status = bd_connection.get_users_in_dialog(
                database_interface::Dialog(new_message.m_dialog_id), users
            );
            send_response_and_return_if_false(
                users_status.correct(), user_connection, SEND_MESSAGE_FAIL,
                "Not able to send file: cannot get users in dialog " +
                    users_status.message()
            );
            std::cout << "here-" << 4 << "\n";
            for (auto &user : users) {
                Status status_get_login =
                    bd_connection.get_user_log_by_id(user);
                std::cout << "here-" << 5 << "\n";
                send_response_and_return_if_false(
                    status_get_login.correct(), user_connection,
                    SEND_MESSAGE_FAIL,
                    "Not able to send file: cannot get user login " +
                        status_get_login.message()
                );
                std::cout << "here-" << 6 << "\n";
                std::cout << "login" << user.m_login << "\n";
                std::string file_save_path =
                    "./../bd/Files/users_files/" + user.m_login;
                if (!std::filesystem::exists(file_save_path)) {
                    if (!std::filesystem::create_directory(file_save_path)) {
                        send_response_and_return_if_false(
                            false, user_connection, SEND_MESSAGE_FAIL,
                            "Cannot create directory for save file for this "
                            "user, probably invalid users login"
                        );
                        std::cout << "here-" << 7 << "\n";
                    }
                }
                std::cout << "here-" << 8 << "\n";
                std::cout << "file_save_path" << file_save_path << "\n";
                Status status = file.save(file_save_path);
                std::cout << "here-" << 9 << "\n";
                send_response_and_return_if_false(
                    status.correct(), user_connection, SEND_MESSAGE_FAIL,
                    "Send message exception: cannot save file " +
                        status.message()
                );
            }
        }

        Status current_status = bd_connection.make_message(new_message);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, SEND_MESSAGE_FAIL,
            "Send message exception: " + current_status.message()
        );
        DecryptedRequest response(SEND_MESSAGE_SUCCESS, new_message);
        user_connection.send_secured_request(response);
    }

    void process_change_old_message_request(
        UserConnection &user_connection,
        const DecryptedRequest &request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            CHANGE_MESSAGE_FAIL, "It is necessary to log in!"
        );
        database_interface::Message old_message;
        database_interface::Message new_message;
        try {
            nlohmann::from_json(request.data, old_message);
            nlohmann::from_json(request.data, new_message);
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                CHANGE_MESSAGE_FAIL,
                "Not able to find message to change: cannot parse json "
                "message: bad request or invalid message data: " +
                    static_cast<std::string>(exception.what())
            );
        }

        Status current_status = bd_connection.get_message_by_id(old_message);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, CHANGE_MESSAGE_FAIL,
            "Invalid Message ID: " + current_status.message()
        );
        send_response_and_return_if_false(
            user_connection.user_in_db->m_user_id == old_message.m_user_id,
            user_connection, CHANGE_MESSAGE_FAIL,
            "Change message exception: client id don't matches message owner id"
        );

        old_message.m_text = new_message.m_text;
        current_status = bd_connection.change_message(old_message);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, CHANGE_MESSAGE_FAIL,
            "Change message exception: " + current_status.message()
        );
        DecryptedRequest response(CHANGE_MESSAGE_SUCCESS, old_message);
        user_connection.send_secured_request(response);
    }

    void proces_delete_message_request(
        UserConnection &user_connection,
        const DecryptedRequest &request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            DELETE_MESSAGE_FAIL, "It is necessary to log in!"
        );
        database_interface::Message message;
        try {
            nlohmann::from_json(request.data, message);
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                SEND_MESSAGE_FAIL,
                "Cannot delete message: cannot parse json message: bad request "
                "or invalid message data: " +
                    static_cast<std::string>(exception.what())
            );
        }
        Status message_get_status = bd_connection.get_message_by_id(message);
        send_response_and_return_if_false(
            message_get_status, user_connection, DELETE_MESSAGE_FAIL,
            "Invalid message id: " + message_get_status.message()
        );
        send_response_and_return_if_false(
            user_connection.user_in_db->m_user_id == message.m_user_id,
            user_connection, DELETE_MESSAGE_FAIL,
            "Delete message exception: client id don't matches message owner id"
        );
        Status current_status = bd_connection.del_message(message);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, DELETE_MESSAGE_FAIL,
            "Delete message exception: " + current_status.message()
        );
        user_connection.send_secured_request(
            DecryptedRequest(DELETE_MESSAGE_SUCCESS, {})
        );
    }

    void process_get_n_messages_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            GET_100_MESSAGES_FAIL, "It is necessary to log in!"
        );
        database_interface::Dialog current_dialog;
        std::list<database_interface::Message> messages_list;
        int number_of_messages;
        int last_message_time;
        try {
            nlohmann::from_json(request.data["dialog"], current_dialog);
            number_of_messages = request.data["number_of_messages"];
            last_message_time = request.data["last_message_time"];
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                GET_100_MESSAGES_FAIL,
                "Cannot get messages: cannot parse json: bad request or "
                "invalid message data: " +
                    static_cast<std::string>(exception.what())
            );
        }

        Status current_status;
        current_status = bd_connection.get_n_dialogs_messages_by_time(
            current_dialog, messages_list, number_of_messages, last_message_time
        );

        send_response_and_return_if_false(
            current_status.correct(), user_connection, GET_100_MESSAGES_FAIL,
            "Get messages exception: " + current_status.message()
        );
        std::vector<json> message_vec(
            messages_list.begin(), messages_list.end()
        );
        user_connection.send_secured_request(DecryptedRequest(
            GET_100_MESSAGES_SUCCESS, nlohmann::json{{"messages", message_vec}}
        ));
    }

    void process_get_n_dialogs_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            GET_100_CHATS_FAIL, "It is necessary to log in!"
        );

        int n_dialogs;
        int last_dialog_time;

        try {
            n_dialogs = request.data["n_dialogs"];
            last_dialog_time = request.data["last_dialog_time"];
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                GET_100_CHATS_FAIL,
                "Cannot get dialogs: cannot parse json: bad request or invalid "
                "message data: " +
                    static_cast<std::string>(exception.what())
            );
        }

        database_interface::User &user = user_connection.user_in_db.value();
        std::list<database_interface::Dialog> dialog_list;
        Status current_status = bd_connection.get_n_users_dialogs_by_time(
            user, dialog_list, n_dialogs, last_dialog_time
        );
        send_response_and_return_if_false(
            current_status.correct(), user_connection, GET_100_CHATS_FAIL,
            "Get dialogs exception: " + current_status.message()
        );

        std::vector<json> json_dialog_vector(
            dialog_list.begin(), dialog_list.end()
        );
        user_connection.send_secured_request(DecryptedRequest(
            GET_100_CHATS_SUCCESS,
            nlohmann::json{{"dialogs", json_dialog_vector}}
        ));
    }

    void process_make_grope_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            MAKE_GROPE_FAIL, "It is necessary to log in!"
        );
        std::vector<int> user_ids;

        database_interface::Dialog new_dialog;
        try {
            new_dialog = request.data["dialog"];
            new_dialog.m_owner_id = user_connection.user_in_db->m_user_id;
            user_ids = std::vector<int>(request.data["user_ids"]);
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                MAKE_GROPE_FAIL,
                "Cannot make grope: cannot parse json: bad request or invalid "
                "data: " +
                    static_cast<std::string>(exception.what())
            );
        }

        std::vector<database_interface::User> user_vec;
        for (auto id : user_ids) {
            user_vec.emplace_back(id);
        }

        Status current_status = bd_connection.make_dialog(new_dialog);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, MAKE_GROPE_FAIL,
            "Make dialog exception: " + current_status.message()
        );
        current_status =
            bd_connection.add_users_to_dialog(user_vec, new_dialog);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, MAKE_GROPE_FAIL,
            "Add users to dialog exception: " + current_status.message()
        );

        user_connection.send_secured_request(
            DecryptedRequest(MAKE_GROPE_SUCCESS, new_dialog)
        );
    }

    static void process_upload_file_request(
        UserConnection &user_connection,
        const DecryptedRequest &request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            FILE_UPLOAD_FAIL, "It is necessary to log in!"
        );
        FileWorker::File file(FileWorker::empty_file);
        try {
            file = FileWorker::File(request.data, FileWorker::parse_JSON);
        } catch (FileWorker::file_exception &exception) {
            send_response_and_return_if_false(
                false, user_connection, FILE_UPLOAD_FAIL, exception.what()
            );
        }
        std::string file_directory_path =
            "./../bd/Files/users_files/" + user_connection.user_in_db->m_login;
        if (!std::filesystem::exists(file_directory_path)) {
            // Creating a directory
            if (!std::filesystem::create_directory(file_directory_path)) {
                send_response_and_return_if_false(
                    false, user_connection, FILE_UPLOAD_FAIL,
                    "Cannot create directory for this user, probably invalid "
                    "users login"
                );
            }
        }
        Status status = file.save(file_directory_path);
        send_response_and_return_if_false(
            status, user_connection, FILE_UPLOAD_FAIL, status.message()
        );
        user_connection.send_secured_request(DecryptedRequest(
            FILE_UPLOAD_SUCCESS, json{{"file_name", status.message()}}
        ));
    }

    static void process_download_file_request(
        UserConnection &user_connection,
        const DecryptedRequest &request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            FILE_DOWNLOAD_FAIL, "It is necessary to log in!"
        );
        send_response_and_return_if_false(
            request.data["file_name"].is_string(), user_connection,
            FILE_DOWNLOAD_FAIL, "File_name should be a string!"
        );
        std::string file_name = request.data["file_name"];
        std::string file_directory_path =
            "./../bd/Files/users_files/" + user_connection.user_in_db->m_login;
        send_response_and_return_if_false(
            std::filesystem::exists(file_directory_path), user_connection,
            FILE_DOWNLOAD_FAIL, "This file does not exist!"
        );
        std::string file_path = file_directory_path + "/" + file_name;
        send_response_and_return_if_false(
            std::filesystem::exists(file_path), user_connection,
            FILE_DOWNLOAD_FAIL, "This file does not exist!"
        );
        FileWorker::File file(FileWorker::empty_file);
        try {
            file = FileWorker::File(file_path);
        } catch (FileWorker::file_exception &exception) {
            send_response_and_return_if_false(
                false, user_connection, FILE_DOWNLOAD_FAIL, exception.what()
            );
        }
        user_connection.send_secured_request(
            DecryptedRequest(FILE_DOWNLOAD_SUCCESS, nlohmann::json(file))
        );
    }

    void process_change_user_request(
        UserConnection &user_connection,
        const DecryptedRequest &request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            CHANGE_USER_FAIL, "It is necessary to log in!"
        );

        database_interface::User old_user(*user_connection.user_in_db);
        auto old_user_status = bd_connection.get_user_by_id(old_user);
        send_response_and_return_if_false(
            old_user_status.correct(), user_connection, CHANGE_USER_FAIL,
            "Change user, problem in getting user by id: " +
                old_user_status.message()
        );
        database_interface::User new_user = old_user;
        std::string change_in;
        try {
            nlohmann::from_json(request.data["old_id"], new_user.m_user_id);
            nlohmann::from_json(request.data["changing_in"], change_in);
            if (change_in == "name") {
                nlohmann::from_json(
                    request.data["new_parametr"], new_user.m_name
                );
            } else if (change_in == "surname") {
                nlohmann::from_json(
                    request.data["new_parametr"], new_user.m_surname
                );
            } else if (change_in == "login") {
                nlohmann::from_json(
                    request.data["new_parametr"], new_user.m_login
                );
            } else if (change_in == "password") {
                nlohmann::from_json(
                    request.data["new_parametr"], new_user.m_password_hash
                );
            } else if (change_in == "encryption") {
                nlohmann::from_json(
                    request.data["new_parametr"], new_user.m_encryption
                );
            }
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                CHANGE_USER_FAIL,
                "Not able to change user info: cannot parse json user: bad "
                "request or invalid user data: " +
                    static_cast<std::string>(exception.what())
            );
        }
        send_response_and_return_if_false(
            old_user.m_user_id == new_user.m_user_id, user_connection,
            CHANGE_USER_FAIL, "You can't make change for another user!"
        )
            send_response_and_return_if_false(
                new_user.m_login.find_first_of("\t\n ") == std::string::npos,
                user_connection, CHANGE_USER_FAIL,
                "Login should contain only one word!"
            );
        send_response_and_return_if_false(
            new_user.m_password_hash.find_first_of("\t\n ") ==
                std::string::npos,
            user_connection, CHANGE_USER_FAIL,
            "Password should contain only one word!"
        );

        Status status = bd_connection.change_user(new_user);
        if (status) {
            old_user = std::move(new_user);
            user_connection.send_secured_request(
                DecryptedRequest(CHANGE_USER_SUCCESS, old_user)
            );
            std::cout << "User with id: " + std::to_string(old_user.m_user_id) +
                             " was changed!\n";
        } else {
            user_connection.send_secured_exception(
                CHANGE_USER_FAIL, "Change user exception: " + status.message()
            );
        }
    }

    void process_get_user_by_login_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        std::string login = request.data["login"];
        send_response_and_return_if_false(
            login.find_first_of("\t\n ") == std::string::npos, user_connection,
            GET_USER_BY_LOGIN_FAIL, "Login should contain one word!"
        );
        database_interface::User user(login);
        Status current_status = bd_connection.get_user_id_by_log(user);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, GET_USER_BY_LOGIN_FAIL,
            "Get user exception: " + current_status.message()
        );
        user_connection.send_secured_request(
            DecryptedRequest(GET_USER_BY_LOGIN_SUCCESS, user)
        );
    }

    void process_get_user_by_id_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            request.data["user_id"].is_number(), user_connection,
            GET_USER_BY_ID_FAIL, "User id should be Integer!"
        );
        database_interface::User user(static_cast<int>(request.data["user_id"])
        );
        Status current_status = bd_connection.get_user_by_id(user);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, GET_USER_BY_ID_FAIL,
            "Get user exception: " + current_status.message()
        );
        user_connection.send_secured_request(
            DecryptedRequest(GET_USER_BY_ID_SUCCESS, user)
        );
    }

    void process_add_user_to_dialog_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            ADD_USER_TO_DIALOG_FAIL, "It is necessary to log in!"
        );
        send_response_and_return_if_false(
            request.data["user_id"].is_number(), user_connection,
            ADD_USER_TO_DIALOG_FAIL, "User_id should be the integer!!"
        );
        send_response_and_return_if_false(
            request.data["dialog_id"].is_number(), user_connection,
            ADD_USER_TO_DIALOG_FAIL, "Dialog_id should be the integer!!"
        );
        database_interface::User user_to_add(
            static_cast<int>(request.data["user_id"])
        );
        database_interface::Dialog current_dialog(
            static_cast<int>(request.data["dialog_id"])
        );
        Status current_status =
            bd_connection.add_user_to_dialog(user_to_add, current_dialog);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, ADD_USER_TO_DIALOG_FAIL,
            "Add_user_to_dialog exception: " + current_status.message()
        );
        user_connection.send_secured_request(
            DecryptedRequest(ADD_USER_TO_DIALOG_SUCCESS, {})
        );
    }

    void process_del_user_from_dialog_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            DEL_USER_FROM_DIALOG_FAIL, "It is necessary to log in!"
        );
        int dialog_id;
        int user_id;
        try {
            nlohmann::from_json(request.data["dialog_id"], dialog_id);
            nlohmann::from_json(request.data["user_id"], user_id);
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                DEL_USER_FROM_DIALOG_FAIL,
                "Not able to del user in dialog: cannot parse json user: bad "
                "request or invalid dialog id, user id: " +
                    static_cast<std::string>(exception.what())
            );
        }
        database_interface::User user_to_add(user_id);
        database_interface::Dialog current_dialog(dialog_id);
        Status current_status =
            bd_connection.del_user_from_dialog(user_to_add, current_dialog);
        send_response_and_return_if_false(
            current_status.correct(), user_connection,
            DEL_USER_FROM_DIALOG_FAIL,
            "Del_user_from_dialog exception: " + current_status.message()
        );
        user_connection.send_secured_request(
            DecryptedRequest(DEL_USER_FROM_DIALOG_SUCCESS, {})
        );
    }

    void process_get_dialog_by_id_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            request.data["dialog_id"].is_number(), user_connection,
            GET_DIALOG_BY_ID_FAIL, "Dialog_id should be the integer!!"
        );
        database_interface::Dialog current_dialog(
            static_cast<int>(request.data["dialog_id"])
        );
        Status current_status = bd_connection.get_dialog_by_id(current_dialog);
        send_response_and_return_if_false(
            current_status.correct(), user_connection, GET_DIALOG_BY_ID_FAIL,
            "Get_dialog_by_id exception: " + current_status.message()
        );
        user_connection.send_secured_request(
            DecryptedRequest(GET_DIALOG_BY_ID_SUCCESS, current_dialog)
        );
    }

    void process_get_users_in_dialog_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            user_connection.get_user_in_db_ref().has_value(), user_connection,
            GET_USERS_IN_DIALOG_FAIL, "It is necessary to log in!"
        );
        send_response_and_return_if_false(
            request.data["dialog_id"].is_number(), user_connection,
            GET_USERS_IN_DIALOG_FAIL, "Dialog_id should be the integer!!"
        );
        std::vector<database_interface::User> users;
        int dialog_id;
        try {
            nlohmann::from_json(request.data["dialog_id"], dialog_id);
        } catch (std::exception &exception) {
            user_connection.send_secured_exception(
                CHANGE_USER_FAIL,
                "Not able to get users in dialog: cannot parse json user: bad "
                "request or invalid dialog id: " +
                    static_cast<std::string>(exception.what())
            );
        }
        Status current_status = bd_connection.get_users_in_dialog(
            database_interface::Dialog(dialog_id), users
        );
        // AWARE: костыль, надо, чтобы это добавили в DB
        for (auto &user : users) {
            auto get_user_status = bd_connection.get_user_by_id(user);
            user.m_password_hash = "";
            send_response_and_return_if_false(
                get_user_status.correct(), user_connection,
                GET_USERS_IN_DIALOG_FAIL,
                "Get_users_in_dialog exception: Get user data by uer id "
                "exception: " +
                    get_user_status.message()
            );
        }
        send_response_and_return_if_false(
            current_status.correct(), user_connection, GET_USERS_IN_DIALOG_FAIL,
            "Get_users_in_dialog exception: " + current_status.message()
        );
        user_connection.send_secured_request(
            DecryptedRequest(GET_USERS_IN_DIALOG_SUCCESS, json(users))
        );
    }

    void process_get_encryption_by_id_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        send_response_and_return_if_false(
            request.data["encryption_id"].is_number(), user_connection,
            GET_ENCRYPTION_FAIL, "Encryption_id should be the integer!!"
        );
        int encryption_id = static_cast<int>(request.data["encryption_id"]);
        std::string encryption_name;
        auto status = bd_connection.get_encryption_name_by_id(
            encryption_id, encryption_name
        );
        if (status) {
            user_connection.send_secured_request(
                DecryptedRequest(GET_ENCRYPTION_SUCCESS, encryption_name)
            );
        } else {
            user_connection.send_secured_exception(
                GET_ENCRYPTION_FAIL, status.message()
            );
        }
    }

    void process_get_all_encryption_request(
        UserConnection &user_connection,
        DecryptedRequest request
    ) {
        std::vector<std::pair<int, std::string>> encryption_id_name;
        auto status =
            bd_connection.get_encryption_pairs_id_name(encryption_id_name);
        if (status) {
            user_connection.send_secured_request(
                DecryptedRequest(GET_ALL_ENCRYPTION_SUCCESS, encryption_id_name)
            );
        } else {
            user_connection.send_secured_exception(
                GET_ALL_ENCRYPTION_FAIL, status.message()
            );
        }
    }

    int find_empty_connection_number() {
        std::cout << "Searching for empty connection number\n";
        std::mt19937 rng(
            (uint32_t)std::chrono::steady_clock::now().time_since_epoch().count(
            )
        );
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

    void send_file(
        UserConnection &user_connection,
        const database_interface::Message &new_message
    ) {
        std::vector<database_interface::User> users;
        FileWorker::File file(FileWorker::empty_file);
        std::string file_path = "./../bd/Files/users_files/" +
                                user_connection.user_in_db->m_login + "/" +
                                new_message.m_file_name;
        try {
            file = FileWorker::File(file_path);
        } catch (FileWorker::file_exception &exception) {
            send_response_and_return_if_false(
                false, user_connection, SEND_MESSAGE_FAIL, exception.what()
            );
        }
        send_response_and_return_if_false(
            std::filesystem::exists(file_path), user_connection,
            SEND_MESSAGE_FAIL, "This file does not exist!"
        );

        Status users_status = bd_connection.get_users_in_dialog(
            database_interface::Dialog(new_message.m_dialog_id), users
        );
        send_response_and_return_if_false(
            users_status.correct(), user_connection, SEND_MESSAGE_FAIL,
            "Not able to send file: cannot get users in dialog " +
                users_status.message()
        );
        for (auto &user : users) {
            Status status_get_login = bd_connection.get_user_log_by_id(user);
            send_response_and_return_if_false(
                status_get_login.correct(), user_connection, SEND_MESSAGE_FAIL,
                "Not able to send file: cannot get user login " +
                    status_get_login.message()
            );
            std::cout << "login" << user.m_login << "\n";
            std::string file_save_path =
                "./../bd/Files/users_files/" + user.m_login;
            if (!std::filesystem::exists(file_save_path)) {
                if (!std::filesystem::create_directory(file_save_path)) {
                    send_response_and_return_if_false(
                        false, user_connection, SEND_MESSAGE_FAIL,
                        "Cannot create directory for save file for this user, "
                        "probably invalid users login"
                    );
                }
            }
            std::cout << "file_save_path" << file_save_path << "\n";
            Status status = file.save(file_save_path);
            send_response_and_return_if_false(
                status.correct(), user_connection, SEND_MESSAGE_FAIL,
                "Send message exception: cannot save file " + status.message()
            );
        }
    }
};

void UserConnection::accept_client_request(
    const std::string &rem_endpoint_str,
    UserConnection &connection
) {
    if (!connection.connection_is_protected ||
        !connection.decrypter.has_value() || connection.client.value().bad()) {
        throw std::runtime_error("Bad connection!");
    }
    std::unique_lock lock(*connection.client_connection_in_mutex);
    EncryptedRequest encrypted_request(connection.client.value());
    lock.unlock();
    std::cout << "Got request from --->>> " << rem_endpoint_str << "\n";
    DecryptedRequest decrypted_request =
        encrypted_request.decrypt(connection.decrypter.value());
    connection.server.push_request_to_queue(
        connection.connection_number, std::move(decrypted_request)
    );
}

void UserConnection::work_with_connection(
    boost::asio::ip::tcp::socket &&socket,
    UserConnection &connection
) {
    auto rem_endpoint = socket.remote_endpoint();
    std::cout << "Accepted connection " << rem_endpoint << " --> "
              << socket.local_endpoint() << "\n";
    client = boost::asio::ip::tcp::iostream(std::move(socket));
    while (true) {
        try {
            std::unique_lock lock(*client_connection_in_mutex);
            EncryptedRequest encrypted_request(client.value());
            lock.unlock();
            DecryptedRequest request =
                encrypted_request.reinterpret_cast_to_decrypted();
            if (request.request_type !=
                MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY) {
                continue;
            }
            connection.decrypter = Cryptographer::Decrypter(
                Cryptographer::Cryptographer::get_rng(),
                request.data["encryption_name"]
            );
            connection.encrypter = Cryptographer::Encrypter(
                request.data["public_key"],
                Cryptographer::Cryptographer::get_rng()
            );
            DecryptedRequest response_with_key(
                MAKE_SECURE_CONNECTION_SUCCESS_RETURN_OTHER_KEY
            );
            response_with_key.data["public_key"] =
                connection.decrypter.value().get_str_publicKey();
            lock.lock();
            Net::try_send_request(
                response_with_key.reinterpret_cast_to_encrypted(),
                client.value()
            );
            EncryptedRequest response(client.value());
            lock.unlock();
            if (response.reinterpret_cast_to_decrypted().request_type !=
                MAKE_SECURE_CONNECTION_SUCCESS) {
                continue;
            }
            connection.connection_is_protected = true;
            std::cout << "Secured connection with " << rem_endpoint
                      << " was established!\n";
        } catch (...) { continue; }
        break;
    }
    bool logging_in_is_successful = false;
    while (!logging_in_is_successful) {
        try {
            std::unique_lock lock(*client_connection_in_mutex);
            EncryptedRequest encrypted_request(client.value());
            lock.unlock();
            DecryptedRequest request =
                encrypted_request.decrypt(decrypter.value());
            if (request.request_type == LOG_IN_REQUEST) {
                server.log_in(connection, request, logging_in_is_successful);
            } else if (request.request_type == SIGN_UP_REQUEST) {
                // Create new user request
                server.sign_up(connection, request);
            }
        } catch (std::exception &exception) {
            std::cerr << "Logging in failed with exception: " +
                             static_cast<std::string>(exception.what()) + "\n";
            return;
        }
    }
    connection_is_active = true;
    std::cout << "Start accepting requests!\n";
    try {
        while (client.value() && connection_is_active) {
            accept_client_request(
                rem_endpoint.address().to_string(), connection
            );
        }
    } catch (...) {
        std::cout << "Connection -> expires, closing the connection!\n";
    }

    std::cout << "Completed -> " << rem_endpoint << "\n";
    // AWARE func calls thread detach!
    server.close_connection(connection_number);
}
}  // namespace Net::Server

#endif  // MESSENGER_PROJECT_NETSERVER_HPP
