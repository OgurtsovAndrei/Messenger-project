//
// Created by andrey on 14.05.23.
//

#ifndef MESSENGER_PROJECT_FILEWORKER_HPP
#define MESSENGER_PROJECT_FILEWORKER_HPP

#include <string>
#include <vector>
#include <fstream>
#include <botan/base64.h>
#include <nlohmann/json.hpp>

namespace FileWorker {

    struct Parse_JSON {
    } parseJson;

    struct File {

        explicit File(nlohmann::json json_data, Parse_JSON &) {
            if (!json_data["file"].is_string() || !json_data["file_name"].is_string()) {
                throw std::runtime_error("Bad file format: expected string!");
            }
            data = unlock(Botan::base64_decode(json_data["file"]));
            file_name = json_data["file_name"];
        }

        explicit File(const std::string &file_path) {
            std::ifstream file(file_path, std::ios_base::in | std::ios_base::binary | std::ios::ate);
            file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            std::size_t file_size = file.tellg();
            file.seekg(0, std::ios::beg);
            data = std::vector<uint8_t>(file_size);
            file.read(reinterpret_cast<char *>(data.data()), static_cast<long>(file_size));
            file_name = file_path;
        }

        explicit operator nlohmann::json() const {
            nlohmann::json json_file;
            json_file["file_name"] = file_name;
            json_file["file"] = convert_file_body_to_string();
            return std::move(json_file);
        }

        friend bool operator==(const File& a, const File& b){
            return a.data == b.data && a.file_name == b.file_name;
        }


    private:
        std::string file_name;
        std::vector<unsigned char> data;

        [[nodiscard]] std::string convert_file_body_to_string() const {
            return Botan::base64_encode(data);
        }

        explicit File(std::string name, std::vector<uint8_t> data_) : file_name(std::move(name)),
                                                                      data(std::move(data_)) {}

    };


}

#endif //MESSENGER_PROJECT_FILEWORKER_HPP
