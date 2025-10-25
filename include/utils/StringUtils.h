#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

class StringUtils {
public:
    static std::string trim(const std::string& str);
    static std::string trim_left(const std::string& str);
    static std::string trim_right(const std::string& str);
    static std::string trim_all(const std::string& str);
    static std::string to_lower(const std::string& str);
    static std::string to_upper(const std::string& str);
    static std::string to_title_case(const std::string& str);
    static std::string to_camel_case(const std::string& str);
    static std::string to_snake_case(const std::string& str);

    static std::vector<std::string> split(const std::string& str, char delimiter);
    static std::vector<std::string> split(const std::string& str, const std::string& delimiter);
    static std::string join(const std::vector<std::string>& strings, const std::string& delimiter);
    static std::string join(const std::vector<std::string>& strings, char delimiter);

    static bool starts_with(const std::string& str, const std::string& prefix);
    static bool ends_with(const std::string& str, const std::string& suffix);
    static bool contains(const std::string& str, const std::string& substring);
    static bool contains_ignore_case(const std::string& str, const std::string& substring);
    static size_t count_occurrences(const std::string& str, char ch);
    static size_t count_occurrences(const std::string& str, const std::string& substring);

    static std::string replace(const std::string& str, const std::string& from, const std::string& to);
    static std::string replace_all(const std::string& str, const std::string& from, const std::string& to);
    static std::string remove(const std::string& str, const std::string& substring);
    static std::string remove_all(const std::string& str, const std::string& substring);

    static bool is_empty(const std::string& str);
    static bool is_blank(const std::string& str);
    static bool is_number(const std::string& str);
    static bool is_alpha(const std::string& str);
    static bool is_alphanumeric(const std::string& str);
    static bool is_whitespace(const std::string& str);

    static std::string html_escape(const std::string& str);
    static std::string html_unescape(const std::string& str);
    static std::string url_encode(const std::string& str);
    static std::string url_decode(const std::string& str);

    static std::string format(const std::string& format, ...);
    static std::string pad_left(const std::string& str, size_t length, char pad_char = ' ');
    static std::string pad_right(const std::string& str, size_t length, char pad_char = ' ');
    static std::string pad_center(const std::string& str, size_t length, char pad_char = ' ');

    static std::string substring(const std::string& str, size_t start, size_t length = std::string::npos);
    static std::string left(const std::string& str, size_t length);
    static std::string right(const std::string& str, size_t length);

    static std::string reverse(const std::string& str);
    static std::string capitalize(const std::string& str);
    static std::string uncapitalize(const std::string& str);

    static bool equals(const std::string& a, const std::string& b);
    static bool equals_ignore_case(const std::string& a, const std::string& b);
    static int compare_ignore_case(const std::string& a, const std::string& b);

    static std::vector<std::string> wrap_text(const std::string& text, size_t line_length);
    static std::string repeat(const std::string& str, size_t times);
    static std::string truncate(const std::string& str, size_t length, const std::string& ellipsis = "...");

    static int to_int(const std::string& str, int default_value = 0);
    static long to_long(const std::string& str, long default_value = 0);
    static double to_double(const std::string& str, double default_value = 0.0);
    static bool to_bool(const std::string& str, bool default_value = false);
    static std::string from_int(int value);
    static std::string from_long(long value);
    static std::string from_double(double value);
    static std::string from_bool(bool value);


    static std::string random(size_t length, const std::string& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
};
inline bool StringUtils::is_empty(const std::string& str) {
    return str.empty();
}

inline bool StringUtils::is_blank(const std::string& str) {
    return std::all_of(str.begin(), str.end(), [](unsigned char c) {
        return std::isspace(c);
    });
}

inline bool StringUtils::equals(const std::string& a, const std::string& b) {
    return a == b;
}