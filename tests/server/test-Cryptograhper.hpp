//
// Created by andrey on 24.05.23.
//

#ifndef MESSENGER_PROJECT_TEST_CRYPTOGRAHPER_HPP
#define MESSENGER_PROJECT_TEST_CRYPTOGRAHPER_HPP

#include <cxxtest/TestSuite.h>
#include "Cryptographer.hpp"

class TestCryptographer : public CxxTest::TestSuite {
public:
    void testInit(void) {
        Cryptographer::Cryptographer cryptographer;
        Cryptographer::Decrypter decrypter(cryptographer.get_rng());
        Cryptographer::Encrypter encrypter(decrypter.get_str_publicKey(), cryptographer.get_rng());
    }

    void testInit2(void) {
        Cryptographer::Cryptographer cryptographer;
        Cryptographer::Decrypter decrypter(cryptographer.get_rng());
        Cryptographer::Encrypter encrypter(decrypter.get_str_publicKey(), cryptographer.get_rng());
    }

    void testEncryptionDecryption(void) {
        Cryptographer::Cryptographer cryptographer;
        Cryptographer::Decrypter decrypter(cryptographer.get_rng());
        Cryptographer::Encrypter encrypter(decrypter.get_str_publicKey(), cryptographer.get_rng());

        auto encrypt_decrypt_test_lambda = [&](std::string plaintext) {
            std::string encrypted_data_enc = encrypter.encrypt_text_to_text(plaintext);
            std::string encrypted_data_dec = decrypter.encrypt_text_to_text(plaintext);

            std::string decrypted_text_enc = Cryptographer::as<std::string>(decrypter.decrypt_data(encrypted_data_enc));
            std::string decrypted_text_dec = Cryptographer::as<std::string>(decrypter.decrypt_data(encrypted_data_dec));

            TS_ASSERT_EQUALS(decrypted_text_dec, plaintext);
            TS_ASSERT_EQUALS(decrypted_text_enc, plaintext);
        };

        const std::vector<std::string> texts {
                "The quick brown fox jumps over the lazy dog.",
                "This file specifies macros that customize the test runner, and output is generated before and after the tests are run.\n"
                "Note that CxxTest needs to insert certain definitions and #include directives in the runner file. It normally does that before\n"
                "the first #include <cxxtest/*.h> found in the template file. If this behavior is not what you need, use the directive\n"
                "<CxxTest preamble> to specify where this preamble is inserted.",
                "Краткое полутехническое описание проекта\n"
                "Главное меню:\n"
                "Основное окно со списком чатов и глобальным поиском. При нажатии ПКМ на чат открывается выбор между удалением, очисткой истории, открытием в новом окне, добавлением пароля и настройки шифрования. При использовании ПКМ на пользователя, появляется запрос “отправить предложение переписки”, если он принят открываются настройки чата + добавляется возможность пригласить пользователя в уже созданную беседу и заблокировать его. \n"
                "Чаты:\n"
                "Чат выглядит следующим образом: пишется имя пользователя (выделяя его на общем фоне), затем текст сообщения с обозначенными границами сообщения. Настройки чата те же, что и при нажатии ПКМ.\n"
                "Беседы:\n"
                "В беседах реализована базовая иерархия пользователей. Существует отдельная кнопка на главном окне для создания беседы, после нажатия которой будет предложено ввести название и выбрать пользователей (назначение прав происходит в настройках самой беседы, по умолчанию создается только владелец). Другие настройки беседы: добавление и удаление пользователей, смена названия + стандартные настройки чата.\n"
                "Шифрование:\n"
                "Пользователь получает возможность выбора между несколькими уже реализованными методами шифрования.\n"
                "Аутентификация:\n"
                "При начальном запуске приложения пользователю предлагается возможность зарегистрироваться (логин, пароль два раза) или авторизоваться (логин и пароль). "
        };

        for (auto text : texts) {
            encrypt_decrypt_test_lambda(text);
        }

    }
};

#endif //MESSENGER_PROJECT_TEST_CRYPTOGRAHPER_HPP
