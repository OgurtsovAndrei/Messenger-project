//
// Created by andrey on 05.05.23.
//

#ifndef MESSENGER_PROJECT_NETGENERAL_HPP
#define MESSENGER_PROJECT_NETGENERAL_HPP


#include <string>
#include <iostream>
#include <cassert>
#include <cstring>
#include <utility>
#include <vector>
#include <utility>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include "Cryptographer.hpp"
#include "Status.hpp"
#include "TextWorker.hpp"

using json = nlohmann::json;

namespace Net {

    enum RequestType {
        TEXT_REQUEST = 1,
        SECURED_REQUEST,
        FILE,
        RESPONSE_REQUEST_SUCCESS,
        RESPONSE_REQUEST_FAIL,
        MAKE_SECURE_CONNECTION_SEND_PUBLIC_KEY,
        MAKE_SECURE_CONNECTION_SUCCESS_RETURN_OTHER_KEY,
        MAKE_SECURE_CONNECTION_SUCCESS,
        MAKE_SECURE_CONNECTION_FAIL,
        LOG_IN_REQUEST,
        LOG_IN_SUCCESS,
        LOG_IN_FAIL,
        GET_100_CHATS,
        GET_100_CHATS_SUCCESS,
        GET_100_CHATS_FAIL,
        SEND_DIALOG_REQUEST,
        GET_ALL_DIALOG_REQUESTS,
        DENY_DIALOG_REQUEST,
        ACCEPT_DIALOG_REQUEST,
        MAKE_GROPE,
        MAKE_GROPE_SUCCESS,
        MAKE_GROPE_FAIL,
        DELETE_DIALOG,
        SEND_MESSAGE,
        SEND_MESSAGE_SUCCESS,
        SEND_MESSAGE_FAIL,
        CHANGE_MESSAGE,
        CHANGE_MESSAGE_SUCCESS,
        CHANGE_MESSAGE_FAIL,
        DELETE_MESSAGE,
        DELETE_MESSAGE_SUCCESS,
        DELETE_MESSAGE_FAIL,
        GET_100_MESSAGES,
        GET_100_MESSAGES_SUCCESS,
        GET_100_MESSAGES_FAIL,
        GET_USER_BY_LOGIN,
        GET_USER_BY_LOGIN_SUCCESS,
        GET_USER_BY_LOGIN_FAIL,
        SIGN_UP_REQUEST,
        SIGN_UP_SUCCESS,
        SIGN_UP_FAIL,
        CLOSE_CONNECTION,
        UNKNOWN,
    };

    inline constexpr int REQUEST_STRING_SIZE_IN_CHARS = 16;

    inline constexpr char request_sep[] = "@\1\3\1#";
    inline constexpr char request_begin[] = "^\1\3\1#";
    inline constexpr char request_end[] = "$\1\3\1#";

    [[nodiscard]] inline std::string read_n_and_get_string(unsigned int n, boost::asio::ip::tcp::iostream &client) {
        char *string_ptr = new char[n + 1];
        string_ptr[n] = '\0';
        client.read(string_ptr, n);
        std::string string;
        for (int i = 0; i < n; ++i) {
            string += string_ptr[i];
        }
        delete[] string_ptr;
        assert(string.size() == n);
        return std::move(string);
    }

    [[nodiscard]] inline std::string convert_to_string_size_n(unsigned int value, unsigned int size = 16) {
        auto str_value = std::to_string(value);
        assert(str_value.size() <= size);
        std::string answer(size, '0');
        answer += str_value;
        return answer.substr(answer.size() - size, size);
    }

    struct EncryptedRequest;

    struct DecryptedRequest {
        friend struct EncryptedRequest;
        RequestType request_type;
        json data{};

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(DecryptedRequest, request_type, data);

        explicit DecryptedRequest(RequestType request_type_, json data_) : request_type(request_type_),
                                                                           data(std::move(data_)) {}

        explicit DecryptedRequest(RequestType request_type_) : request_type(request_type_) {}

        [[nodiscard]] EncryptedRequest encrypt(const Cryptographer::Encrypter &encrypter) const;

        [[nodiscard]] EncryptedRequest reinterpret_cast_to_encrypted() const;

        RequestType get_type() const { return request_type; }

    private:
        explicit DecryptedRequest() : request_type(UNKNOWN) {}
    };

    struct EncryptedRequest {

        explicit EncryptedRequest(boost::asio::ip::tcp::iostream &connection) {
            std::string request_string_size = read_n_and_get_string(REQUEST_STRING_SIZE_IN_CHARS, connection);
            if (!is_number(request_string_size)) {std::cout << "<<" <<  request_string_size << ">>\n"; }
            if (!is_number(request_string_size)) {
                throw std::runtime_error("Bad connection! Expected request length, but received bytes are not number!");
            }
            int request_size = std::stoi(request_string_size);
            data_string = read_n_and_get_string(request_size, connection);
        }

        [[nodiscard]] DecryptedRequest decrypt(Cryptographer::Decrypter &decrypter) const {
            // CAREFUL, funk throws the exceptions!!!
            auto decrypted_string = decrypter.decrypt_data(data_string);
            json json_data = json::parse(decrypted_string);
            DecryptedRequest request{};
            nlohmann::from_json(json_data, request);
            return request;
        }

        [[nodiscard]] std::string to_text() const {
            std::string text_request;
            text_request += convert_to_string_size_n(data_string.size(), REQUEST_STRING_SIZE_IN_CHARS);
            text_request += data_string;
            return std::move(text_request);
        }

        [[nodiscard]] DecryptedRequest reinterpret_cast_to_decrypted() const {
            // CAREFUL, funk throws the exceptions!!!
            json json_data = json::parse(data_string);
            DecryptedRequest request;
            nlohmann::from_json(json_data, request);
            return request;
        }

    private:
        friend struct DecryptedRequest;
        std::string data_string;

        explicit EncryptedRequest(std::string str_) : data_string(std::move(str_)) {}

    };

    inline EncryptedRequest DecryptedRequest::encrypt(const Cryptographer::Encrypter &encrypter) const {
        json other = (*this);
        return EncryptedRequest(encrypter.encrypt_text_to_text(other.dump()));
    }

    inline EncryptedRequest DecryptedRequest::reinterpret_cast_to_encrypted() const {
        json other = (*this);
        return EncryptedRequest(other.dump());
    }

    namespace {
        inline void send_text_to_server(const std::string &text, boost::asio::ip::tcp::iostream &connection) {
            connection << text;
        }
    }

    inline void try_send_request(const EncryptedRequest &request, boost::asio::ip::tcp::iostream &connection) {
        send_text_to_server(request.to_text(), connection);
    }
}

#endif //MESSENGER_PROJECT_NETGENERAL_HPP
