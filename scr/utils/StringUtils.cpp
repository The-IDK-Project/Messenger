#include "utils/StringUtils.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>

std::string StringUtils::trim(const std::string& str) {
    return trim_right(trim_left(str));
}

std::string StringUtils::trim_left(const std::string& str) {
    auto it = std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    return std::string(it, str.end());
}

std::string StringUtils::trim_right(const std::string& str) {
    auto it = std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base();
    return std::string(str.begin(), it);
}

std::string StringUtils::to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string StringUtils::to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::vector<std::string> StringUtils::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;

    while (std::getline(stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

std::vector<std::string> StringUtils::split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }

    tokens.push_back(str.substr(start));
    return tokens;
}

std::string StringUtils::join(const std::vector<std::string>& strings, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < strings.size(); ++i) {
        if (i > 0) oss << delimiter;
        oss << strings[i];
    }
    return oss.str();
}

std::string StringUtils::join(const std::vector<std::string>& strings, char delimiter) {
    return join(strings, std::string(1, delimiter));
}

bool StringUtils::starts_with(const std::string& str, const std::string& prefix) {
    if (prefix.length() > str.length()) return false;
    return std::equal(prefix.begin(), prefix.end(), str.begin());
}

bool StringUtils::ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

bool StringUtils::contains(const std::string& str, const std::string& substring) {
    return str.find(substring) != std::string::npos;
}

bool StringUtils::contains_ignore_case(const std::string& str, const std::string& substring) {
    std::string str_lower = to_lower(str);
    std::string substring_lower = to_lower(substring);
    return contains(str_lower, substring_lower);
}

size_t StringUtils::count_occurrences(const std::string& str, char ch) {
    return std::count(str.begin(), str.end(), ch);
}

std::string StringUtils::replace(const std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos) return str;

    std::string result = str;
    result.replace(start_pos, from.length(), to);
    return result;
}

std::string StringUtils::replace_all(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;

    std::string result = str;
    size_t start_pos = 0;
    while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
        result.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return result;
}

bool StringUtils::is_number(const std::string& str) {
    if (str.empty()) return false;

    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

std::string StringUtils::pad_left(const std::string& str, size_t length, char pad_char) {
    if (str.length() >= length) return str;
    return std::string(length - str.length(), pad_char) + str;
}

std::string StringUtils::pad_right(const std::string& str, size_t length, char pad_char) {
    if (str.length() >= length) return str;
    return str + std::string(length - str.length(), pad_char);
}

std::string StringUtils::pad_center(const std::string& str, size_t length, char pad_char) {
    if (str.length() >= length) return str;

    size_t padding = length - str.length();
    size_t left_padding = padding / 2;
    size_t right_padding = padding - left_padding;

    return std::string(left_padding, pad_char) + str + std::string(right_padding, pad_char);
}

std::string StringUtils::reverse(const std::string& str) {
    std::string result = str;
    std::reverse(result.begin(), result.end());
    return result;
}

std::string StringUtils::capitalize(const std::string& str) {
    if (str.empty()) return str;

    std::string result = str;
    result[0] = std::toupper(result[0]);
    return result;
}

bool StringUtils::equals_ignore_case(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    return to_lower(a) == to_lower(b);
}

int StringUtils::compare_ignore_case(const std::string& a, const std::string& b) {
    std::string a_lower = to_lower(a);
    std::string b_lower = to_lower(b);
    return a_lower.compare(b_lower);
}

std::vector<std::string> StringUtils::wrap_text(const std::string& text, size_t line_length) {
    std::vector<std::string> lines;
    if (text.empty() || line_length == 0) return lines;

    std::istringstream words(text);
    std::string word;
    std::string current_line;

    while (words >> word) {
        if (current_line.length() + word.length() + 1 > line_length) {
            if (!current_line.empty()) {
                lines.push_back(trim(current_line));
                current_line.clear();
            }

            while (word.length() > line_length) {
                lines.push_back(word.substr(0, line_length));
                word = word.substr(line_length);
            }
        }

        if (!current_line.empty()) current_line += " ";
        current_line += word;
    }

    if (!current_line.empty()) {
        lines.push_back(trim(current_line));
    }

    return lines;
}

std::string StringUtils::repeat(const std::string& str, size_t times) {
    std::string result;
    result.reserve(str.length() * times);
    for (size_t i = 0; i < times; ++i) {
        result += str;
    }
    return result;
}

std::string StringUtils::truncate(const std::string& str, size_t length, const std::string& ellipsis) {
    if (str.length() <= length) return str;
    return str.substr(0, length - ellipsis.length()) + ellipsis;
}

int StringUtils::to_int(const std::string& str, int default_value) {
    try {
        return std::stoi(str);
    } catch (...) {
        return default_value;
    }
}

double StringUtils::to_double(const std::string& str, double default_value) {
    try {
        return std::stod(str);
    } catch (...) {
        return default_value;
    }
}

bool StringUtils::to_bool(const std::string& str, bool default_value) {
    std::string lower_str = to_lower(trim(str));

    if (lower_str == "true" || lower_str == "1" || lower_str == "yes" || lower_str == "on") {
        return true;
    } else if (lower_str == "false" || lower_str == "0" || lower_str == "no" || lower_str == "off") {
        return false;
    }

    return default_value;
}

std::string StringUtils::from_int(int value) {
    return std::to_string(value);
}

std::string StringUtils::from_double(double value) {
    return std::to_string(value);
}

std::string StringUtils::from_bool(bool value) {
    return value ? "true" : "false";
}