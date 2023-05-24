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
#include <Status.hpp>

namespace FileWorker {

    struct ParseJSON {
    } parse_JSON;

    struct EmptyFile {} empty_file;

    struct file_exception : std::runtime_error {
        explicit file_exception(const std::string& str) : std::runtime_error(str) {}
    };

    struct bad_file_name_exception : file_exception {
        explicit bad_file_name_exception(const std::string& string) : file_exception(string) {};
    };

    struct invalid_file_format_exception : file_exception {
        explicit invalid_file_format_exception(const std::string& string) : file_exception(string) {};
    };

    struct read_file_exception : file_exception {
        explicit read_file_exception(const std::string& string) : file_exception(string) {};
    };

    struct File {

        explicit File(EmptyFile &) {}

        explicit File(nlohmann::json json_data, ParseJSON &) {
            if (!json_data["file"].is_string() || !json_data["file_name"].is_string()) {
                throw invalid_file_format_exception("Bad file format: in given json['file'] expected string!");
            }
            data = unlock(Botan::base64_decode(json_data["file"]));
            file_name = remove_extra_file_name_head(json_data["file_name"]);
        }

        explicit File(const std::string &file_path) {
            std::ifstream file(file_path, std::ios_base::in | std::ios_base::binary | std::ios::ate);
            file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            try {
                std::size_t file_size = file.tellg();
                file.seekg(0, std::ios::beg);
                data = std::vector<uint8_t>(file_size);
                file.read(reinterpret_cast<char *>(data.data()), static_cast<long>(file_size));
            } catch (std::ios_base::failure &exception) {
                throw read_file_exception("Unable to rad the file: " + std::string(exception.what()));
            }
            file_name = remove_extra_file_name_head(file_path);
        }

        explicit operator nlohmann::json() const {
            nlohmann::json json_file;
            json_file["file_name"] = file_name;
            json_file["file"] = convert_file_body_to_string();
            return std::move(json_file);
        }

        friend bool operator==(const File &a, const File &b) {
            return a.data == b.data && a.file_name == b.file_name;
        }

        [[nodiscard]] Status save(const std::string &file_save_directory) const {
            if (!check_file_name_is_correct(file_name)) {
                return Status(false, "Filename has one of '\n\t /\"' to close to end of the file");
            }
            try {
                std::string save_file_path = file_save_directory + "/" + file_name;
                std::ofstream out_file(
                        save_file_path, std::ios_base::out | std::ios_base::binary
                );
                out_file.exceptions(std::ios_base::failbit | std::ios_base::badbit);
                out_file.write(
                        reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size())
                );
            } catch (std::ios_base::failure &exception) {
                return Status(false, "Write file exception: " + std::string(exception.what()));
            }
            return Status(true, file_name);
        }

        [[nodiscard]] std::string get_name() const {
            return file_name;
        }

        Status change_name(const std::string& new_name) {
            if (check_file_name_is_correct(new_name)) {
                file_name = new_name;
                return Status(true);
            }
            return Status(false, "Bad new file name!");
        }

    private:
        std::string file_name;
        std::vector<unsigned char> data;

        [[nodiscard]] std::string convert_file_body_to_string() const {
            return Botan::base64_encode(data);
        }

        explicit File(const std::string& name, std::vector<uint8_t> data_) : file_name(remove_extra_file_name_head(name)),
                                                                      data(std::move(data_)) {}

        static std::string remove_extra_file_name_head (const std::string& name) {
            // Throws the exception!
            auto slash_id = name.find_last_of("\n\t /\"");
            if (slash_id == std::string::npos) {
                return name;
            } else {
                if (slash_id == name.size() - 1) {
                    throw bad_file_name_exception("Filename has one of '\n\t /\"' to close to end of the file");
                }
                return name.substr(slash_id + 1);
            }
        }

        static bool check_file_name_is_correct(const std::string &file_name) {
            return file_name.find_first_of("\n\t /\"") == std::string::npos && !file_name.empty();
        }
    };

}

#endif //MESSENGER_PROJECT_FILEWORKER_HPP
