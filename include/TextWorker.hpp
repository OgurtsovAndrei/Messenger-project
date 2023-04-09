//
// Created by andrey on 19.03.23.
//

#ifndef MESSENGER_PROJECT_TEXTWORKER_HPP
#define MESSENGER_PROJECT_TEXTWORKER_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <cassert>

inline constexpr unsigned int INT_VECTOR_SIZE_SIZE_IN_CHARS = 10;
inline constexpr unsigned int INT_SIZE_SIZE_IN_CHARS = 10;
inline constexpr unsigned int TEXT_VECTOR_SIZE_SIZE_IN_CHARS = 10;
inline constexpr unsigned int TEXT_VECTOR_ELEMENT_SIZE_IN_CHARS = 10;

inline std::string convert_to_string_size_n(unsigned int value, unsigned int size = 4) {
    auto str_value = std::to_string(value);
    assert(str_value.size() <= size);
    std::string answer(size, '0');
    answer += str_value;
    return answer.substr(answer.size() - size, size);
}

[[nodiscard]] inline std::string convert_int_vector_to_text(const std::vector<unsigned int> &int_vec) {
    std::string answer;
    answer += convert_to_string_size_n(int_vec.size(), INT_VECTOR_SIZE_SIZE_IN_CHARS);
    for (const unsigned int element: int_vec) {
        answer += convert_to_string_size_n(element, INT_SIZE_SIZE_IN_CHARS);
    }
    return std::move(answer);
};

[[nodiscard]] inline std::vector<unsigned int> convert_to_int_vector_from_text(const std::string &text) {
    std::vector<unsigned int> answer;
    unsigned int vector_size = std::stoi(text.substr(0, INT_VECTOR_SIZE_SIZE_IN_CHARS));
    unsigned int covered_len = INT_VECTOR_SIZE_SIZE_IN_CHARS;
    for (auto index = 0; index < vector_size; ++index) {
        unsigned int element = std::stoi(text.substr(covered_len, INT_SIZE_SIZE_IN_CHARS));
        covered_len += INT_SIZE_SIZE_IN_CHARS;
        answer.push_back(element);
    }
    return std::move(answer);
};

[[nodiscard]] inline std::string convert_text_vector_to_text(const std::vector<std::string> &text_vec) {
    std::string answer;
    answer += convert_to_string_size_n(text_vec.size(), TEXT_VECTOR_SIZE_SIZE_IN_CHARS);
    for (const std::string &element: text_vec) {
        answer += convert_to_string_size_n(element.size(), TEXT_VECTOR_ELEMENT_SIZE_IN_CHARS);
        answer += element;
    }
    return std::move(answer);
};

[[nodiscard]] inline std::vector<std::string> convert_to_text_vector_from_text(const std::string &text) {
    std::vector<std::string> answer;
    unsigned int vector_size = std::stoi(text.substr(0, TEXT_VECTOR_SIZE_SIZE_IN_CHARS));
    unsigned int covered_len = TEXT_VECTOR_SIZE_SIZE_IN_CHARS;
    for (auto index = 0; index < vector_size; ++index) {
        unsigned int element_size = std::stoi(text.substr(covered_len, TEXT_VECTOR_ELEMENT_SIZE_IN_CHARS));
        covered_len += TEXT_VECTOR_SIZE_SIZE_IN_CHARS;
        std::string element = text.substr(covered_len, element_size);
        answer.push_back(element);
        covered_len += element_size;
    }
    return std::move(answer);
};

[[nodiscard]] inline bool is_number(const std::string &s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

#endif //MESSENGER_PROJECT_TEXTWORKER_HPP
