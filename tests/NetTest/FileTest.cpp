//
// Created by andrey on 14.05.23.
//
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include "FileWorker.hpp"
#include <filesystem>

using json = nlohmann::json;

int main()
{
    // create a binary vector
    std::vector<std::uint8_t> vec = {0xCA, 0xFE, 0xBA, 0xBE};

    // create a binary JSON value with subtype 42
    json j = json::binary(vec, 42);

    // output type and subtype
    std::cout << "type: " << j.type_name() << ", subtype: " << j.get_binary().subtype() << std::endl;
    std::cout << j.dump() << std::endl;

    std::filesystem::path cwd = std::filesystem::current_path(); // "filename.txt";
    std::cout << cwd << std::endl;
//    std::ofstream file1(cwd.string());
//    file1.close();

    FileWorker::File file("./../bd/Files/Lena.bmp");
    json json_file(file);
    std::cout << json_file.dump(4) << std::endl;

    FileWorker::File returned_file(json_file, FileWorker::parse_JSON);
    assert(returned_file == file);
    auto status = file.save("./../bd/Files/saved");
    if (!status.correct()) {
        std::cerr << "Cannot save_file: " << status.message() << std::endl;
    }
    return 0;
}