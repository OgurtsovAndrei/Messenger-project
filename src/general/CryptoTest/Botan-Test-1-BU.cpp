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

#include "Cryptographer.hpp"

using namespace Cryptographer;

int main()
{
    Botan::AutoSeeded_RNG rng;

    const auto key_pair = generate_keypair(2048 /*  bits */, rng);

    std::string privateKey = Botan::base64_encode(Botan::PKCS8::BER_encode(*key_pair));
    std::string publicKey = Botan::base64_encode(Botan::X509::BER_encode(*key_pair->public_key()));

    std::cout << "Private Key = " <<privateKey << "\n\n";
    std::cout << "Public Key = " << privateKey << "\n\n";

    const std::string plaintext = "The quick brown fox jumps over the lazy dog.";
    const auto ciphertext       = encrypt(as<Botan::secure_vector<uint8_t>>(plaintext), key_pair->public_key(), rng);
    const auto new_plaintext    = decrypt(ciphertext, *key_pair, rng);


//    std::cout << "Ciphertext: " << as<std::string>(ciphertext.ciphertext) << std::endl;

    std::cout << as<std::string>(new_plaintext) << std::endl;

    //Loading the public key
    Botan::SecureVector<uint8_t> publicKeyBytes(Botan::base64_decode(publicKey));
    std::unique_ptr<Botan::Public_Key> pbk(Botan::X509::load_key(std::vector(publicKeyBytes.begin(), publicKeyBytes.end())));

    //Loading the private key
    Botan::SecureVector<uint8_t> privateKeyBytes(Botan::base64_decode(privateKey));
    Botan::DataSource_Memory source(privateKeyBytes);
    std::unique_ptr<Botan::Private_Key> pvk(Botan::PKCS8::load_key(source));

    const auto new_plaintext_after_save_key = decrypt(ciphertext, *pvk, rng);
    std::cout << as<std::string>(new_plaintext_after_save_key) << std::endl;

    return 0;
}