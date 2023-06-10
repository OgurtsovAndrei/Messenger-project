//
// Created by andrey on 19.03.23.
//

#ifndef MESSENGER_PROJECT_CRYPTOGRAPHER_HPP
#define MESSENGER_PROJECT_CRYPTOGRAPHER_HPP

#include <iostream>
#include <string>
#include <cassert>


#include <botan/aead.h>
#include <botan/rsa.h>
#include <botan/types.h>
#include <botan/auto_rng.h>
#include <botan/dsa.h>
#include <botan/secmem.h>
#include <botan/cipher_mode.h>
#include <botan/pubkey.h>
#include <botan/base64.h>
#include <botan/pkcs8.h>
#include <botan/x509_key.h>
#include <botan/data_src.h>
#include "TextWorker.hpp"

#include <botan/auto_rng.h>
#include <botan/dl_group.h>
#include <botan/rng.h>
#include <botan/dh.h>
#include <botan/dsa.h>
#include <botan/elgamal.h>
#include <botan/ec_group.h>
#include <botan/gost_3410.h>
#include <botan/dlies.h>
#include <botan/ecdsa.h>
#include <botan/ecies.h>


namespace Cryptographer {
    template<typename Out, typename In>
    Out as(const In &data) {
        return Out(data.data(), data.data() + data.size());
    }

    struct EncryptedData {
        Botan::secure_vector<uint8_t> ciphertext;
        std::vector<uint8_t> nonce;
        std::vector<uint8_t> encryptedKey;
    };

    [[nodiscard]] inline std::vector<std::string> convert_encrypted_data_to_text_vector(const EncryptedData &data_block) {
        auto text = as<std::string>(data_block.ciphertext);
        auto nonce = as<std::string>(data_block.nonce);
        auto key = as<std::string>(data_block.encryptedKey);
        std::vector<std::string> text_vector = {text, nonce, key};
        return text_vector;
    }

    [[nodiscard]] inline EncryptedData make_encrypted_data_from_text_vector(const std::vector<std::string> &text_vec) {
        assert(text_vec.size() == 3);
        auto text = as<Botan::secure_vector<uint8_t>>(text_vec[0]);
        auto nonce = as<std::vector<uint8_t>>(text_vec[1]);
        auto key = as<std::vector<uint8_t>>(text_vec[2]);
        return {text, nonce, key};
    }

    [[nodiscard]] inline EncryptedData make_encrypted_data_from_text(const std::string &text) {
        return make_encrypted_data_from_text_vector(convert_to_text_vector_from_text(text));
    }

    [[nodiscard]] inline  std::string convert_encrypted_data_to_text(const EncryptedData &data_block) {
        return convert_text_vector_to_text(convert_encrypted_data_to_text_vector(data_block));
    }

    inline std::unique_ptr<Botan::Private_Key>
    generate_keypair_RSA(const size_t bits,
                     Botan::RandomNumberGenerator &rng) {
        return std::make_unique<Botan::RSA_PrivateKey>(rng, bits);
    }

    inline std::unique_ptr<Botan::Private_Key>
    generate_keypair_DSA(const size_t bits,
                             Botan::RandomNumberGenerator &rng) {
        return std::make_unique<Botan::ElGamal_PrivateKey>(rng, Botan::DL_Group("dsa/jce/1024"));
    }

    inline std::unique_ptr<Botan::Private_Key>
    generate_keypair_DH(const size_t bits,
                         Botan::RandomNumberGenerator &rng) {
        return std::make_unique<Botan::ElGamal_PrivateKey>(rng, Botan::DL_Group("dsa/jce/1024"));
    }


    inline EncryptedData encrypt(const Botan::secure_vector<uint8_t> &data,
                          Botan::Public_Key *pubkey,
                          Botan::RandomNumberGenerator &rng) {
        // Создаём симметричный ключ, которым будем шифровать основной текст
        auto sym_cipher = Botan::AEAD_Mode::create_or_throw("AES-256/GCM", Botan::Cipher_Dir::Encryption);
        // Куда записываем результат
        EncryptedData d;

        // prepare random key material for the symmetric encryption/authentication
        rng.random_vec(d.nonce, sym_cipher->default_nonce_length());
        const auto key = rng.random_vec(sym_cipher->minimum_keylength());
        d.ciphertext = data;

        // encrypt/authenticate the data symmetrically
        sym_cipher->set_key(key);
        sym_cipher->start(d.nonce);
        sym_cipher->finish(d.ciphertext);

        // encrypt the symmetric key using RSA
        // Botan::ECIES_Encryptor asym_cipher(rng, Botan::ECIES_System_Params());
        Botan::PK_Encryptor_EME asym_cipher(*pubkey, rng, "EME-OAEP(SHA-256,MGF1)");
        d.encryptedKey = asym_cipher.encrypt(key, rng);

        return d;
    }

    inline EncryptedData encrypt(const Botan::secure_vector<uint8_t> &data,
                          std::unique_ptr<Botan::Public_Key> pubkey,
                          Botan::RandomNumberGenerator &rng) {
        return encrypt(data, &*pubkey, rng);
    }


    Botan::secure_vector<uint8_t>
    inline decrypt(const EncryptedData &encdata,
            const Botan::Private_Key &privkey,
            Botan::RandomNumberGenerator &rng) {
        // prepare random key material for the symmetric encryption/authentication
        Botan::secure_vector<uint8_t> plaintext = encdata.ciphertext;

        // decrypt the symmetric key
        Botan::PK_Decryptor_EME asym_cipher(privkey, rng, "EME-OAEP(SHA-256,MGF1)");
        const auto key = asym_cipher.decrypt(encdata.encryptedKey);

        // decrypt the data symmetrically
        auto sym_cipher = Botan::AEAD_Mode::create_or_throw("AES-256/GCM", Botan::Cipher_Dir::Decryption);
        sym_cipher->set_key(key);
        sym_cipher->start(encdata.nonce);
        sym_cipher->finish(plaintext);

        return plaintext;
    }

    struct Decrypter;

    struct Encrypter;

    struct Cryptographer {
    public:
        static Botan::AutoSeeded_RNG &get_rng() {
            static Botan::AutoSeeded_RNG rng;
            return rng;
        }
    };

    struct Decrypter {
    public:
        Decrypter(const Decrypter &con) = delete;

        Decrypter(Decrypter &&) = default;

        Decrypter &operator=(const Decrypter &) = delete;

        Decrypter &operator=(Decrypter &&other) noexcept {
            key_pair = std::move(other.key_pair);
            return *this;
        };

        explicit Decrypter(Botan::AutoSeeded_RNG &rng_, std::string encryption_name) : rng(rng_) {
            if (encryption_name == "RSA"){
                key_pair = generate_keypair_RSA(1024 /*  bits */, rng_);
            } else if (encryption_name == "DSA") {
                key_pair = generate_keypair_DSA(1024 /*  bits */, rng_);
            } else {
                key_pair = generate_keypair_DH(1024 /*  bits */, rng_);
            }
        };

        [[nodiscard]] std::string get_str_publicKey() const {
            return Botan::base64_encode(Botan::X509::BER_encode(*key_pair->public_key()));
        };

        [[nodiscard]] std::unique_ptr<Botan::Public_Key> get_public_key() const {
            return key_pair->public_key();
        }

        [[nodiscard]] EncryptedData encrypt_text_to_encrypted_data_block(const std::string &plaintext) const {
            return encrypt(as<Botan::secure_vector<uint8_t>>(plaintext), get_public_key(), rng);
        }

        [[nodiscard]] std::string encrypt_text_to_text(const std::string &plaintext) const {
            return std::move(convert_encrypted_data_to_text(encrypt_text_to_encrypted_data_block(plaintext)));
        }

        [[nodiscard]] auto decrypt_data(const EncryptedData &ciphertext) const {
            return decrypt(ciphertext, *key_pair, rng);
        }

        [[nodiscard]] auto decrypt_data(const std::string &ciphertext) const {
            return decrypt_data(make_encrypted_data_from_text(ciphertext));
        }


    private:
        Botan::AutoSeeded_RNG &rng;

        std::unique_ptr<Botan::Private_Key> key_pair;
    };

    struct Encrypter {
    public:
        Encrypter(const Encrypter &) = delete;

        Encrypter(Encrypter &&) = default;

        Encrypter &operator=(const Encrypter &) = delete;

        Encrypter &operator=(Encrypter &&other) noexcept {
            public_key = std::move(other.public_key);
            return *this;
        };

        explicit Encrypter(const std::string &public_key_str, Botan::AutoSeeded_RNG &rng_) : rng(rng_) {
            Botan::SecureVector<uint8_t> publicKeyBytes(Botan::base64_decode(public_key_str));
            public_key = std::unique_ptr<Botan::Public_Key>(
                    Botan::X509::load_key(std::vector(publicKeyBytes.begin(), publicKeyBytes.end())));
        };

        [[nodiscard]] std::string get_str_publicKey() const {
            return Botan::base64_encode(Botan::X509::BER_encode(*public_key));
        };

        [[nodiscard]] Botan::Public_Key *get_public_key() const {
            return &*public_key;
        }

        [[nodiscard]] EncryptedData encrypt_text_to_encrypted_data_block(const std::string &plaintext) const {
            return encrypt(as<Botan::secure_vector<uint8_t>>(plaintext), get_public_key(), rng);
        }

        [[nodiscard]] std::string encrypt_text_to_text(const std::string &plaintext) const {
            return std::move(convert_encrypted_data_to_text(encrypt_text_to_encrypted_data_block(plaintext)));
        }

    private:
        Botan::AutoSeeded_RNG &rng;
        std::unique_ptr<Botan::Public_Key> public_key;
    };


}

#endif //MESSENGER_PROJECT_CRYPTOGRAPHER_HPP
