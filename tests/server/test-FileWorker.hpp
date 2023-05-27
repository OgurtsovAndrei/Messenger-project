//
// Created by andrey on 28.05.23.
//

#ifndef MESSENGER_PROJECT_TEST_FILEWORKER_HPP
#define MESSENGER_PROJECT_TEST_FILEWORKER_HPP

#include <cxxtest/TestSuite.h>
#include <iostream>
#include <type_traits>
#include "FileWorker.hpp"

using namespace FileWorker;

class TestFileWorker : public CxxTest::TestSuite {
    using File = FileWorker::File;

public:
    const std::vector<std::string> file_names = {
            "Lena.bmp",
            "The Starry Night.jpeg",
            "Night.jpg",
            "CxxTest-guide.pdf"
    };

    const std::string working_dir_name = "./test-FileWorker-directory/";

    void testInit(void) {
        TS_ASSERT(!std::is_default_constructible<FileWorker::File>::value);
        TS_ASSERT_THROWS_NOTHING(File file1(FileWorker::empty_file));
        for (auto file_name: file_names) {
            File file2("./test-data/" + file_name);
        }
        // Wierd Clion behaviour, actually everything is OK
        TS_ASSERT_THROWS(File file2("./test-data/123.txt"), read_file_exception)
        TS_ASSERT_THROWS(File file3("./"), read_file_exception)
        File file4(nlohmann::json{{"file_name", "MyFile"},
                                  {"file",      ""}},  FileWorker::parse_JSON);
        TS_ASSERT_THROWS(File file5(nlohmann::json{{"file_name", "MyFile"},
                                                   {"file",      123}}, FileWorker::parse_JSON), invalid_file_format_exception);
        TS_ASSERT_THROWS(File file6(nlohmann::json{{"file_name", "MyFile"},
                                                   {"file",      1.0}}, FileWorker::parse_JSON), invalid_file_format_exception);
        TS_ASSERT_THROWS(File file7(nlohmann::json{{"file_name", 456},
                                                   {"file",      ""}}, FileWorker::parse_JSON), invalid_file_format_exception);
        TS_ASSERT_THROWS(File file8(nlohmann::json{{"file_name", 239},
                                                   {"file",      566}}, FileWorker::parse_JSON), invalid_file_format_exception);
    }

    void testConversations() {
        for (auto file_name: file_names) {
            File file1("./test-data/" + file_name);
            nlohmann::json json_file = file1.operator nlohmann::json();
            File file2(json_file, parse_JSON);
            TS_ASSERT_EQUALS(file1, file2);
        }
    }

};

#endif //MESSENGER_PROJECT_TEST_FILEWORKER_HPP
