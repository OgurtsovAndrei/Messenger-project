//
// Created by andrey on 08.03.23.
//

#ifndef MESSENGER_PROJECT_NETSERVER_HPP
#define MESSENGER_PROJECT_NETSERVER_HPP

#include <utility>
#include "NetGeneral.hpp"
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
#include <queue>
#include <condition_variable>
#include "Status.hpp"
#include "../../../include/TextWorker.hpp"
#include "./../CryptoTest/Cryptographer.hpp"
#include "./../../../include/database/User.hpp"
#include "./../../../include/database/DataBaseInterface.hpp"

namespace Net::Server {

    struct Server;

    struct UserConnection {

    public:
        explicit UserConnection(boost::asio::ip::tcp::socket &&socket_, Server &server_, int connection_number_) :
                server(server_), connection_number(connection_number_), current_socket(std::move(socket_)) {};

        UserConnection(const UserConnection &con) = delete;

        UserConnection(UserConnection &&) = default;

        UserConnection &operator=(const UserConnection &) = delete;

        UserConnection &operator=(UserConnection &&) = delete;

        void work_with_connection(boost::asio::ip::tcp::socket &&socket, UserConnection &connection);

        void run_connection() {
            session_thread = std::move(std::thread([&, socket = std::move(current_socket), this]() mutable {
                work_with_connection(std::move(socket), *this);
            }));
        }

        [[nodiscard]] bool is_protected() const {
            return connection_is_protected;
        }

        [[nodiscard]] boost::asio::ip::tcp::iostream &get_client_ref() {
            return client.value();
        }

        std::optional<database_interface::User> &get_user_in_db_ref() {
            return user_in_db;
        }

        void send_secured_request(RequestType type, const std::string &request_body) {
            assert(is_protected());
            std::string encrypted_message = encrypter.value().encrypt_text_to_text(request_body);
            Request request(type, std::move(encrypted_message));
            request.make_request();
            assert(try_send_request(request, get_client_ref()));
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
        // Not necessary to init
        std::vector<Request> connection_requests;
        std::optional<boost::asio::ip::tcp::iostream> client;
        std::optional<std::thread> session_thread;
        bool connection_is_protected = false;
        Cryptographer::Cryptographer cryptographer;
        std::optional<Cryptographer::Encrypter> encrypter;
        std::optional<Cryptographer::Decrypter> decrypter;
        std::optional<database_interface::User> user_in_db;

        static void accept_client_request(boost::asio::ip::tcp::iostream &client, const std::string &rem_endpoint_str,
                                          UserConnection &connection);

        Request decrypt_request(Request request) { //NOLINT [static_method]
            if (!request.is_encrypted) {return std::move(request);}
            assert(connection_is_protected);
            auto decrypted_body = Cryptographer::as<std::string>(
                    decrypter.value().decrypt_data(request.get_encrypted_body()));
            request.set_body(decrypted_body);
            request.is_encrypted = false;
            return std::move(request);
        }

        void log_in(UserConnection &connection, Request &request);
    };

    struct RequestQueue {
    public:
        RequestQueue() = default;

        [[nodiscard]] std::pair<int, Request> get_last_or_wait() {
            std::unique_lock queue_lock(queue_mutex);
            while (request_queue.empty()) {
                cond_var.wait(queue_lock);
            }
            auto value = std::move(request_queue.front());
            request_queue.pop();
            return std::move(value);
        }

        void push_to_queue(int number, Request value) {
            std::unique_lock queue_lock(queue_mutex);
            request_queue.emplace(number, std::move(value));
            cond_var.notify_one();
        }

        void push_to_queue(std::pair<int, Request> value) {
            std::unique_lock queue_lock(queue_mutex);
            request_queue.push(std::move(value));
            cond_var.notify_one();
        }

    private:
        std::mutex queue_mutex;
        std::queue<std::pair<int, Request>> request_queue;
        std::condition_variable cond_var; // FIXME: Как лучше назвать? Пока что пусть будет такая заглушка.
    };

    struct Server {
    public:
        explicit Server(const int port_ = 80) : port(port_), connection_acceptor(io_context,
                                                                                 boost::asio::ip::tcp::endpoint(
                                                                                         boost::asio::ip::tcp::v4(),
                                                                                         port_)) {
            std::cout << "Listening at " << connection_acceptor.local_endpoint() << "\n";
        }

        [[nodiscard]] [[maybe_unused]] int get_port() const {
            return port;
        }

        [[noreturn]] void run_server(int number_of_thread_in_pool = 1) {
            for (int consumer_id = 0; consumer_id < number_of_thread_in_pool; ++consumer_id) {
                consumers.push_back(std::move(std::thread([&, this]() mutable {
                    while (true) {
                        // В очереди хранятся уже расшифрованные request-ы
                        auto [connection_id, request] = request_queue.get_last_or_wait();
                        assert(request.is_readable() || request.get_status() == RAW_DATA);
                        std::cout << "Got from user with --> " << connection_id << " connection id: "
                                  << request.get_body() << "\n";
                        std::unique_lock sessions_lock(sessions_mutex);
                        auto iter = sessions.find(connection_id);
                        assert(iter != sessions.end());
                        UserConnection &user_connection = iter->second;
                        sessions_lock.unlock();
                        switch (request.get_type()) {
                            case TEXT_REQUEST:
                                send_message_by_connection(RESPONSE_REQUEST_SUCCESS,
                                                           "Got from you: <" + request.get_body() + ">",
                                                           user_connection.get_client_ref());
                                break;
                            case SECURED_REQUEST:
                                send_secured_request_to_user(RESPONSE_REQUEST_SUCCESS,
                                                             "Got from you: <" + request.get_body() + ">",
                                                             user_connection);
                                break;
                            case FILE:
                                break;
                            case RESPONSE_REQUEST_SUCCESS:
                                break;
                            case RESPONSE_REQUEST_FAIL:
                                break;
                            case MAKE_UNSECURE_CONNECTION:
                                break;
                            case MAKE_UNSECURE_CONNECTION_SUCCESS:
                                break;
                            case MAKE_UNSECURE_CONNECTION_FAIL:
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
                            case GET_100_CHATS:
                                get_n_dialogs(user_connection, std::move(request));
                                break;
                            case GET_USER_BY_LOG_AND_PASSWORD:
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
                                make_grope(user_connection, std::move(request));
                                break;
                            case DELETE_DIALOG:
                                break;
                            case SEND_MESSAGE:
                                process_send_message_request(user_connection, std::move(request));
                                break;
                            case CHANGE_MESSAGE:
                                change_old_message(user_connection, std::move(request));
                                break;
                            case DELETE_MESSAGE:
                                delete_message(user_connection, std::move(request));
                                break;
                            case GET_100_MESSAGES:
                                get_n_messages(user_connection, std::move(request));
                                break;
                            case SIGN_UP_REQUEST:
                                break;
                            case SIGN_UP_SUCCESS:
                                break;
                            case UNKNOWN:
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
            Server &server_ref = *this;
            int empty_number = find_empty_connection_number();
            sessions.emplace(empty_number, UserConnection(std::move(socket), server_ref, empty_number));
            UserConnection &connection = sessions.find(empty_number)->second;
            connection.run_connection();
        }

        void close_connection(int connection_number) {
            assert(sessions.count(connection_number));
            sessions.erase(connection_number);
        }

        Status open_database() {
            return bd_connection.open();
        }

        Status close_database() {
            return bd_connection.close();
        }

        void push_request_to_queue(int number, Request value) {
            request_queue.push_to_queue(number, std::move(value));
        }

        void push_request_to_queue(std::pair<int, Request> value) {
            request_queue.push_to_queue(std::move(value));
        }

        database_interface::SQL_BDInterface &get_bd_connection_ref() {
            return bd_connection;
        }

        void sign_up(UserConnection &user_connection, Request request) {
            std::vector<std::string> data_vector = convert_to_text_vector_from_text(request.get_body());
            assert(data_vector.size() == 4);
            std::string name = data_vector[0];
            std::string surname = data_vector[1];
            std::string login = data_vector[2];
            std::string password = data_vector[3];

            assert(login.find_first_of("\t\n ") == std::string::npos);

            database_interface::User new_user(name, surname, login, password);
            auto status = bd_connection.make_user(new_user);
            if (status) {
                send_secured_request_to_user(SIGN_UP_SUCCESS, std::to_string(new_user.m_user_id), user_connection);
            } else {
                send_secured_request_to_user(SIGN_UP_FAIL, status.message(), user_connection);
            }
        }

        void log_in(UserConnection &connection, Request &request) {
            std::vector<std::string> parsed_body = convert_to_text_vector_from_text(request.get_body());
            assert(parsed_body.size() == 2);
            std::string login = parsed_body[0];
            std::string password = parsed_body[1];
            connection.user_in_db = database_interface::User(login, password);
            database_interface::User &user_in_db = connection.user_in_db.value();
            auto status = bd_connection.get_user_by_log_pas(user_in_db);
            std::cout << "User with id: " + std::to_string(user_in_db.m_user_id) + " logged in!\n";
            if (status) {
                connection.send_secured_request(LOG_IN_SUCCESS, std::to_string(user_in_db.m_user_id));
            } else {
                connection.send_secured_request(LOG_IN_FAIL, "");
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

        void process_send_message_request(UserConnection &user_connection, Request request) {
            std::vector<std::string> data_vector = convert_to_text_vector_from_text(request.get_body());
            assert(data_vector.size() == 3);
            //TODO: check first two are ints
            assert(is_number(data_vector[0]));
            assert(is_number(data_vector[1]));
            int dialog_id = std::stoi(data_vector[0]);
            int current_time = std::stoi(data_vector[1]);
            std::string message_text = data_vector[2];
            auto &user_in_db = user_connection.get_user_in_db_ref();
            assert(user_in_db.has_value());
            database_interface::Message new_message(current_time, message_text, "", dialog_id,
                                                    user_in_db.value().m_user_id);
            bd_connection.make_message(new_message);
        }

        void change_old_message(UserConnection &user_connection, Request request) {
            std::vector<std::string> data_vector = convert_to_text_vector_from_text(request.get_body());
            assert(data_vector.size() == 2);
            // TODO
            assert(is_number(data_vector[0]));
            int old_message_id = std::stoi(data_vector[0]);
            std::string new_body = data_vector[1];
            // TODO
            database_interface::Message old_message(old_message_id);
            Status current_status = bd_connection.get_message_by_id(old_message);
            assert(current_status.correct());
            old_message.m_text = new_body;
            current_status = bd_connection.change_message(old_message);
            assert(current_status.correct());
            send_secured_request_to_user(CHANGE_MESSAGE_SUCCESS, "", user_connection);
        }

        void delete_message(UserConnection &user_connection, Request request) {
            std::vector<std::string> data_vector = convert_to_text_vector_from_text(request.get_body());
            assert(data_vector.size() == 1);
            // TODO
            assert(is_number(data_vector[0]));
            int message_id = std::stoi(data_vector[0]);

            Status current_status = bd_connection.del_message(database_interface::Message(message_id));
            assert(current_status.correct());
            send_secured_request_to_user(CHANGE_MESSAGE_SUCCESS, "", user_connection);
        }

        void get_n_messages(UserConnection &user_connection, Request request) {
            std::vector<std::string> data_vector = convert_to_text_vector_from_text(request.get_body());
            assert(data_vector.size() == 3);
            // TODO
            assert(is_number(data_vector[0]));
            assert(is_number(data_vector[1]));
            assert(is_number(data_vector[2]));
            int number_of_messages = std::stoi(data_vector[0]);
            int dialog_id = std::stoi(data_vector[1]);
            int last_message_time = std::stoi(data_vector[2]);

            database_interface::Dialog current_dialog(dialog_id);
            Status current_status;
            std::list<database_interface::Message> messages_list;

            current_status = bd_connection.get_n_dialogs_messages_by_time(current_dialog, messages_list,
                                                                          number_of_messages, last_message_time);
            assert(current_status);
            std::vector<std::string> message_vec;
            message_vec.resize(number_of_messages);
            for (auto &message : messages_list) {
                message_vec.push_back(message.to_strint());
            }
            send_secured_request_to_user(GET_100_MESSAGES_SUCCESS, convert_text_vector_to_text(message_vec), user_connection);
        }

        void get_n_dialogs(UserConnection &user_connection, Request request) {
            std::vector<std::string> data_vector = convert_to_text_vector_from_text(request.get_body());
            assert(data_vector.size() == 2);
            assert(is_number(data_vector[0]));
            assert(is_number(data_vector[1]));
            int n_dialogs = std::stoi(data_vector[0]);
            int last_dialog_time = std::stoi(data_vector[1]);

            database_interface::User &user = user_connection.user_in_db.value();
            std::list<database_interface::Dialog> dialog_list;
            bd_connection.get_n_users_dialogs_by_time(user, dialog_list, n_dialogs, last_dialog_time);

            std::vector<std::string> str_dialog_vector;
            for (const auto& dialog : dialog_list) {
                str_dialog_vector.push_back(std::move(dialog.to_string()));
            }
            send_secured_request_to_user(GET_100_CHATS_SUCCESS, convert_text_vector_to_text(str_dialog_vector), user_connection);
        }

        void make_grope(UserConnection &user_connection, Request request) {
            std::vector<std::string> data_vector = convert_to_text_vector_from_text(request.get_body());
            assert(data_vector.size() == 5);

            // TODO: Check that vec[4] is correct int vec
            assert(is_number(data_vector[2]));
            assert(is_number(data_vector[3]));

            std::string dialog_name = data_vector[0];
            std::string encryption = data_vector[1];
            int current_time = std::stoi(data_vector[2]);
            int is_grope = std::stoi(data_vector[3]);
            std::vector<unsigned int> user_ids = convert_to_int_vector_from_text(data_vector[4]);

            std::vector<database_interface::User> user_vec;
            for (auto id : user_ids) {
                database_interface::User user(static_cast<int>(id));
                user_vec.push_back(std::move(user));
            }

            auto &user_in_db = user_connection.get_user_in_db_ref();
            assert(user_in_db.has_value());

            database_interface::Dialog new_dialog(dialog_name, encryption, current_time,
                                                  user_connection.user_in_db->m_user_id, is_grope);

            bd_connection.make_dialog(new_dialog);
            bd_connection.add_users_to_dialog(user_vec, new_dialog);

            send_secured_request_to_user(MAKE_GROPE_SUCCESS, std::to_string(new_dialog.m_dialog_id), user_connection);
        }

        int find_empty_connection_number() {
            std::cout << "Searching for empty connection number\n";
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

        static void send_secured_request_to_user(RequestType type, const std::string &request_body,
                                                 UserConnection &user_connection) {
            user_connection.send_secured_request(type, request_body);
        }
    };

    void
    UserConnection::accept_client_request(boost::asio::ip::tcp::iostream &client, const std::string &rem_endpoint_str,
                                          UserConnection &connection) {
        Request request = accept_request(client, connection.connection_is_protected);
        std::cout << "Got request from --->>> " << rem_endpoint_str << "\n";
        request.parse_request();
        assert(request);
        if (connection.connection_is_protected) {
            request = connection.decrypt_request(std::move(request));
        }
        assert(request.is_readable());
        connection.server.push_request_to_queue(connection.connection_number, std::move(request));
    }

    void UserConnection::work_with_connection(boost::asio::ip::tcp::socket &&socket, UserConnection &connection) {
        auto rem_endpoint = socket.remote_endpoint();
        std::cout << "Accepted connection " << rem_endpoint << " --> "
                  << socket.local_endpoint() << "\n";
        client = boost::asio::ip::tcp::iostream(std::move(socket));
        {
            Request request = accept_request(client.value(), connection_is_protected);
            request.parse_request();
            assert(request);
            if (request.get_type() == MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY) {
                connection.decrypter = Cryptographer::Decrypter(Cryptographer::Cryptographer::get_rng());
                connection.encrypter = Cryptographer::Encrypter(request.get_body(),
                                                                Cryptographer::Cryptographer::get_rng());
                send_message_by_connection(MAKE_SECURE_CONNECTION_SUCCESS_RETURN_OTHER_KEY,
                                           connection.decrypter.value().get_str_publicKey(), client.value());
                Request response = accept_request(client.value(), connection_is_protected);
                response.parse_request();
                assert(response);
                assert(response.get_type() == MAKE_SECURE_CONNECTION_SUCCESS);
                connection.connection_is_protected = true;
                std::cout << "Secured connection with " << rem_endpoint << " was established!\n";
            } else {
                assert(request.get_type() == MAKE_UNSECURE_CONNECTION);
                send_message_by_connection(MAKE_UNSECURE_CONNECTION_SUCCESS,
                                           "", client.value());
            }
        }
        {
            Request request = accept_request(client.value(), connection_is_protected);
            request.parse_request();
            assert(request);
            if (connection_is_protected) {
                request = decrypt_request(std::move(request));
            }
            if (request.get_type() == LOG_IN_REQUEST) {
                server.log_in(connection, request);
            } else {
                // Create new user request
                assert(request.get_type() == SIGN_UP_REQUEST);
                server.sign_up(connection, std::move(request));
            }
        }

        std::cout << "Start accepting requests!\n";
        while (client) {
            accept_client_request(client.value(), rem_endpoint.address().to_string(), connection);
        }
        std::cout << "Completed -> " << rem_endpoint << "\n";
        // AWARE func calls thread detach!
        server.close_connection(connection_number);
    }
}

#endif //MESSENGER_PROJECT_NETSERVER_HPP
