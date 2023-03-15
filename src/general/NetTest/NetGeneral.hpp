//
// Created by andrey on 15.03.23.
//

#ifndef MESSENGER_PROJECT_NETGENERAL_HPP
#define MESSENGER_PROJECT_NETGENERAL_HPP

#include <string>
#include <iostream>
#include <vector>

namespace Net {

    enum RequestType {
        TEXT_MESSAGE = 1,
        FILE,
        UNKNOWN,
    };

    enum RequestStatus {
        RAW_DATA,
        RAW_MESSAGE,
        CORRECT,
        INVALID
    };

    inline const int NUMBER_OF_BLOCKS_IN_REQUEST = 3;

    inline constexpr char request_sep[] = "@\1\3\1#";
    inline constexpr char request_begin[] = "^\1\3\1#";
    inline constexpr char request_end[] = "$\1\3\1#";

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
        Request(RequestType request_type_, std::string body_) : request_type(request_type_), body(std::move(body_)), request_status(RAW_DATA) {}

        Request(std::string text_request_, bool) :
        text_request(std::move(text_request_)), request_type(UNKNOWN), request_status(RAW_MESSAGE) {}

        const std::string &get_text_request() {
            if (request_status == CORRECT || request_status == RAW_MESSAGE) {
                return text_request;
            }
        };

        const std::string &get_body() {
            if (request_status == CORRECT || request_status == RAW_DATA) {
                return body;
            }
        };

        void parse_request() {
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
                        text_request.substr(strlen(request_begin), separator_indexes.at(0)- strlen(request_begin))));
                new_body_size = text_request.substr(separator_indexes.at(0) + strlen(request_sep),
                                                    separator_indexes.at(1) - (separator_indexes.at(0) + strlen(request_sep)));
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
            text_request += request_begin;
            text_request += std::to_string(static_cast<int>(request_type));
            text_request += request_sep;
            text_request += std::to_string(body.size());
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

}

#endif //MESSENGER_PROJECT_NETGENERAL_HPP
