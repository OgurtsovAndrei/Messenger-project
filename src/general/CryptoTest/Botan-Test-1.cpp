//
// Created by andrey on 19.02.23.
//

#include <iostream>
#include <string>

#include <botan/aead.h>
#include <botan/rsa.h>
#include <botan/types.h>
#include <botan/auto_rng.h>
#include <botan/secmem.h>
#include <botan/cipher_mode.h>
#include <botan/pubkey.h>
#include <botan/base64.h>
#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/data_src.h>
//#include <botan/vector>

#include "./../../../include/Cryptographer.hpp"

using namespace Cryptographer;

int main()
{
    Botan::AutoSeeded_RNG rng;

    Decrypter decrypter(rng);

    std::string publicKey = decrypter.get_str_publicKey();

    std::cout << "Public Key = " << publicKey << "\n";

    const std::string plaintext = "The quick brown fox jumps over the lazy dog.";
    const std::string plaintext2 = "The quick brown fox jumps over the lazy dog.The quick brown fox jumps over the lazy dog.";

    /*const EncryptedData ciphertext       = encrypter.encrypt_text_to_encrypted_data_block(plaintext);

    const auto new_plaintext    = decrypt(ciphertext, *key_pair, rng);
    std::cout << "Ciphertext: " << as<std::string>(ciphertext.ciphertext) << std::endl;
    std::cout << as<std::string>(new_plaintext) << std::endl;*//*

    auto text = as<std::string>(ciphertext.ciphertext);
    auto nonce = as<std::string>(ciphertext.nonce);
    auto key = as<std::string>(ciphertext.encryptedKey);


    std::cout << nonce << "\n\n" << key << "\n\n";
    std::cout << "DONE1\n";
//    Botan::secure_vector<uint8_t> text_1(Botan::base64_decode(text));
    auto text_1 = as<Botan::secure_vector<uint8_t>>(text);
    std::cout << "DONE2\n";
    auto nonce_1 = as<std::vector<uint8_t>>(nonce);
    std::cout << "DONE3\n";
    auto key_1 = as<std::vector<uint8_t>>(key);
    std::cout << "DONE4\n";


//    EncryptedData ciphertext1{ciphertext.ciphertext, nonce_1, key_1};
    EncryptedData ciphertext1{text_1, nonce_1, key_1};*/

    //Loading the public key
    Botan::SecureVector<uint8_t> publicKeyBytes(Botan::base64_decode(publicKey));
    std::unique_ptr<Botan::Public_Key> pbk(Botan::X509::load_key(std::vector(publicKeyBytes.begin(), publicKeyBytes.end())));

   /* //Loading the private key
//    Botan::SecureVector<uint8_t> privateKeyBytes(Botan::base64_decode(privateKey));
//    Botan::DataSource_Memory source(privateKeyBytes);
//    std::unique_ptr<Botan::Private_Key> pvk(Botan::PKCS8::load_key(source));

//    const auto new_plaintext_after_save_key = decrypt(ciphertext, *pvk, rng);
//    std::cout << as<std::string>(new_plaintext_after_save_key) << std::endl;*/

//    auto x = decrypter.encrypt_text_to_encrypted_data_block(plaintext);
    std::cout << as<std::string>(decrypter.decrypt_data(decrypter.encrypt_text_to_encrypted_data_block(plaintext))) << std::endl;
//
    std::string text_to_send = decrypter.encrypt_text_to_text(plaintext);
//
    std::cout << as<std::string>(decrypter.decrypt_data(text_to_send)) << std::endl;

    return 0;

}