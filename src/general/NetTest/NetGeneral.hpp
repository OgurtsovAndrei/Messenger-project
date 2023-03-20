//
// Created by andrey on 15.03.23.
//

#ifndef MESSENGER_PROJECT_NETGENERAL_HPP
#define MESSENGER_PROJECT_NETGENERAL_HPP

#include <string>
#include <iostream>
#include <cassert>
#include <cstring>
#include <vector>
#include <utility>
#include <boost/asio.hpp>
#include "./../TextWorker.hpp"

namespace Net {

    enum RequestType {
        TEXT_MESSAGE = 1,
        FILE,
        PUBLIC_KEY_SHARE,
        RESPONSE_REQUEST_SUCCESS,
        RESPONSE_REQUEST_FAIL,
        UNKNOWN,
    };

    enum RequestStatus {
        RAW_DATA,
        RAW_MESSAGE,
        CORRECT,
        INVALID
    };

    inline constexpr int NUMBER_OF_BLOCKS_IN_REQUEST = 3;
    inline constexpr int TYPE_SIZE_IN_REQUEST_STRING_IN_CHARS = 16;
    inline constexpr int BODY_SIZE_IN_REQUEST_STRING_IN_CHARS = 16;

    inline constexpr char request_sep[] = "@\1\3\1#";
    inline constexpr char request_begin[] = "^\1\3\1#";
    inline constexpr char request_end[] = "$\1\3\1#";

    std::string convert_to_string_size_n(unsigned int value, unsigned int size = 4) {
        auto str_value = std::to_string(value);
        assert(str_value.size() <= size);
        std::string answer(size, '0');
        answer += str_value;
        return answer.substr(answer.size() - size, size);
    }

    struct Request {
        /*
         * !!! README !!!
         * Данный класс позволяет сконвертировать сообщение, которые надо передать между сервером и пользователем.
         * Делается это следующим образом:
         * Сначала создается класс, если хотим передать сообщение для передачи используем конструктор без bool.
         * Затем вызываем make_request()
         * Если хотим сконвертировать переданное нам сообщение, используем конструктор с bool.
         * Затем вызываем parse_request()
         * Если обратная конвертация не удалась в request_type лежит INVALID
         * TODO: добавить проверку хеша
         */
    public:
        Request(const Request &con) = delete;
        Request(Request &&) = default;
        Request &operator=(const Request &) = delete;
        Request &operator=(Request &&) = default;

        Request(RequestType request_type_, std::string body_) : request_type(request_type_), body(std::move(body_)),
                                                                request_status(RAW_DATA) {}

        Request(std::string text_request_, bool) :
                text_request(std::move(text_request_)), request_type(UNKNOWN), request_status(RAW_MESSAGE) {}

        const std::string &get_text_request() {
            if (request_status == CORRECT || request_status == RAW_MESSAGE) {
                return text_request;
            }
            make_request();
            return text_request;
        };

        const std::string &get_body() {
            if (request_status == CORRECT || request_status == RAW_DATA) {
                return body;
            }
            parse_request();
            return body;
        };

        void parse_request() {
            if (request_status == CORRECT) {
                return;
            }
            std::vector<int> separator_indexes;
            int last_index = 0;
            while (true) {
                auto find_sep_index_result = text_request.find(request_sep, last_index);
                if (find_sep_index_result == std::string::npos) {
                    break;
                }
                separator_indexes.push_back(static_cast<int>(find_sep_index_result));
                last_index = separator_indexes.back() + 1;
            }
            auto find_begin_index_result = text_request.find(request_begin);
            auto find_end_index_result = text_request.find(request_end);

            // Checks:
            if (find_begin_index_result != 0 ||
                find_end_index_result != text_request.size() - 5 ||
                separator_indexes.size() != NUMBER_OF_BLOCKS_IN_REQUEST - 1 ||
                separator_indexes[0] < 6) {
                request_status = INVALID;
                return; // this_request is invalid
            }

            RequestType new_request_type = UNKNOWN;
            std::string new_body_size;
            std::string new_body;

            try { // TODO: remove exceptions
                new_request_type = static_cast<RequestType>(std::stoi(
                        text_request.substr(strlen(request_begin), separator_indexes.at(0) - strlen(request_begin))));
                new_body_size = text_request.substr(separator_indexes.at(0) + strlen(request_sep),
                                                    separator_indexes.at(1) -
                                                    (separator_indexes.at(0) + strlen(request_sep)));
                new_body = text_request.substr(separator_indexes.at(1) + strlen(request_sep),
                                               find_end_index_result - (separator_indexes.at(1) + strlen(request_sep)));
                if (new_body.size() != std::stoi(new_body_size)) {
                    request_status = INVALID;
                    return;
                }
            } catch (...) {
                request_status = INVALID;
                return;
            }

            request_type = new_request_type;
            body = std::move(new_body);
            request_status = CORRECT;
        }

        void make_request() {
            if (request_status == CORRECT) {
                return;
            }
            text_request += request_begin;
            text_request += convert_to_string_size_n(static_cast<int>(request_type),
                                                     TYPE_SIZE_IN_REQUEST_STRING_IN_CHARS);
            text_request += request_sep;
            text_request += convert_to_string_size_n(body.size(), BODY_SIZE_IN_REQUEST_STRING_IN_CHARS);
            text_request += request_sep;
            text_request += body;
            text_request += request_end;
            request_status = CORRECT;
        }

        explicit operator bool() {
            return request_status == CORRECT;
        }

    private:
        RequestType request_type;
        RequestStatus request_status;
        std::string body;
        std::string text_request;
    };

    [[nodiscard]] std::string read_n_and_get_string(unsigned int n, boost::asio::ip::tcp::iostream &client) {
        char *string_ptr = new char[n + 1];
        string_ptr[n] = '\0';
        client.read(string_ptr, n);
        std::string string = string_ptr;
        delete[] string_ptr;
        assert(string.size() == n);
        return std::move(string);
    }

    Request accept_request(boost::asio::ip::tcp::iostream &client) {
        // TODO Очень плохой код! Починить!
        // Но оно работает...

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
        true_string = true_string.substr(true_string.size()-strlen(request_begin), strlen(request_begin));
        assert(true_string.find(request_begin) == true_string.size() - strlen(request_begin));
        true_string = request_begin;

        true_string += read_n_and_get_string(TYPE_SIZE_IN_REQUEST_STRING_IN_CHARS + strlen(request_sep), client);
        assert(true_string.find(request_sep) == true_string.size() - strlen(request_sep));

        std::string new_part = read_n_and_get_string(BODY_SIZE_IN_REQUEST_STRING_IN_CHARS + strlen(request_sep), client);
        assert(new_part.find(request_sep) == new_part.size() - strlen(request_sep));
        true_string += new_part;

        unsigned int body_size = std::stoi(new_part.substr(0, BODY_SIZE_IN_REQUEST_STRING_IN_CHARS));
        true_string += read_n_and_get_string(body_size, client);

        std::string last_request_part = read_n_and_get_string(strlen(request_end), client);
        assert(last_request_part == request_end);
        true_string += last_request_part;
        Request request(true_string, true);
        request.parse_request();
        return std::move(request);
    }

    void send_text_to_server(const std::string &text, boost::asio::ip::tcp::iostream &connection) {
        connection << text << std::endl;
    }

    bool try_send_request(Request& request, boost::asio::ip::tcp::iostream &connection) {
        request.make_request();
        if (request) {
            send_text_to_server(request.get_text_request(), connection);
            return true;
        } else {
            return false;
        }
    }

    std::string get_line_from_connection(boost::asio::ip::tcp::iostream &connection) {
        std::string response;
        std::getline(connection, response);
        return std::move(response);
    }

    void send_message_by_connection(RequestType type, std::string message, boost::asio::ip::tcp::iostream &connection) {
        Request request(type, std::move(message));
        request.make_request();
        if (!try_send_request(request, connection)) {
            return;
        }
    }
}

#endif //MESSENGER_PROJECT_NETGENERAL_HPP
