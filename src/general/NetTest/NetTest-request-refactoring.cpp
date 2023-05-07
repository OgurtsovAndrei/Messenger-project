//
// Created by andrey on 05.05.23.
//

#include "NetGeneral-Refactored.hpp"
#include "Cryptographer.hpp"
#include <iostream>
#include <string>

Botan::AutoSeeded_RNG rng;
struct Message {
    int m_message_id;
    int m_date_time;
    std::string m_text;
    int m_user_id;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Message, m_message_id, m_date_time, m_text, m_user_id);

    friend bool operator==(Message rhs, Message lhs) {
        return rhs.m_message_id == lhs.m_message_id &&  rhs.m_user_id == lhs.m_user_id &&  rhs.m_date_time == lhs.m_date_time &&  rhs.m_text == lhs.m_text;
    }
};

int main() {
    Message message{7, 13, "lalala", 21};
    json other_msg = message;
    Net::DecryptedRequest decrypted_request1(Net::TEXT_REQUEST, other_msg);

    Cryptographer::Decrypter decrypter(rng);
    Cryptographer::Encrypter encrypter(decrypter.get_str_publicKey(), rng);
    auto encrypted_request = decrypted_request1.encrypt(encrypter);
    std::cout << encrypted_request.to_text() << std::endl;
    auto decrypted_request2 = encrypted_request.decrypt(decrypter);
    std::cout << (json) decrypted_request2 << std::endl;
    assert((Message) decrypted_request1.data == (Message)decrypted_request2.data);
}