//
// Created by andrey on 19.03.23.
//

#ifndef MESSENGER_PROJECT_TEXTWORKER_HPP
#define MESSENGER_PROJECT_TEXTWORKER_HPP

#include <vector>
#include <string>

inline constexpr unsigned int TEXT_VECTOR_SIZE_SIZE_IN_CHARS = 10;
inline constexpr unsigned int TEXT_VECTOR_ELEMENT_SIZE_IN_CHARS = 10;

std::string convert_to_string_size_n(unsigned int value, unsigned int size = 4) {
    auto str_value = std::to_string(value);
    assert(str_value.size() <= size);
    std::string answer(size, '0');
    answer += str_value;
    return answer.substr(answer.size() - size, size);
}

[[nodiscard]] std::string convert_text_vector_to_text(const std::vector<std::string>& text_vec) {
    std::string answer;
    answer += convert_to_string_size_n(text_vec.size(), TEXT_VECTOR_SIZE_SIZE_IN_CHARS);
    for (const std::string &element : text_vec) {
        answer += convert_to_string_size_n(element.size(), TEXT_VECTOR_ELEMENT_SIZE_IN_CHARS);
        answer += element;
    }
    return std::move(answer);
};

[[nodiscard]] std::vector<std::string> get_text_vector_from_text(const std::string& text) {
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

#endif //MESSENGER_PROJECT_TEXTWORKER_HPP
