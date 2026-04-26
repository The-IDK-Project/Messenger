#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

class JsonValue {
public:
    enum Type {
        NULL_TYPE,
        BOOL,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT
    };

    JsonValue();
    JsonValue(std::nullptr_t);
    JsonValue(bool value);
    JsonValue(int value);
    JsonValue(int64_t value);
    JsonValue(double value);
    JsonValue(const std::string& value);
    JsonValue(const char* value);
    JsonValue(const std::vector<JsonValue>& value);
    JsonValue(const std::map<std::string, JsonValue>& value);

    // Copy and move
    JsonValue(const JsonValue& other);
    JsonValue(JsonValue&& other) noexcept;
    JsonValue& operator=(const JsonValue& other);
    JsonValue& operator=(JsonValue&& other) noexcept;
    ~JsonValue();

    Type get_type() const;
    bool is_null() const;
    bool is_bool() const;
    bool is_number() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    bool as_bool(bool default_value = false) const;
    int as_int(int default_value = 0) const;
    int64_t as_int64(int64_t default_value = 0) const;
    double as_double(double default_value = 0.0) const;
    std::string as_string(const std::string& default_value = "") const;

    size_t size() const;
    const JsonValue& operator[](size_t index) const;
    JsonValue& operator[](size_t index);
    void push_back(const JsonValue& value);
    void push_back(JsonValue&& value);

    bool has_key(const std::string& key) const;
    const JsonValue& operator[](const std::string& key) const;
    JsonValue& operator[](const std::string& key);
    void set(const std::string& key, const JsonValue& value);
    void set(const std::string& key, JsonValue&& value);
    std::vector<std::string> get_keys() const;

    std::string to_string(bool pretty = false, int indent = 2) const;
    std::string to_json() const;

    static JsonValue parse(const std::string& json_string);
    static JsonValue array();
    static JsonValue object();

private:
    class Impl;
    void serialize(std::ostringstream& oss, bool pretty, int indent, int current_indent) const;
    std::unique_ptr<Impl> impl_;
};

class JsonParser {
public:
    static JsonValue parse(const std::string& json_string);
    static JsonValue parse_file(const std::string& filename);
    static bool validate(const std::string& json_string);

    static std::string stringify(const JsonValue& value, bool pretty = false, int indent = 2);
    static bool write_to_file(const JsonValue& value, const std::string& filename, bool pretty = false);

    class ParseError : public std::runtime_error {
    public:
        ParseError(const std::string& message, size_t position);
        size_t get_position() const;
    private:
        size_t position_;
    };

private:
    static JsonValue parse_value(const std::string& json_string, size_t& position);
    static JsonValue parse_object(const std::string& json_string, size_t& position);
    static JsonValue parse_array(const std::string& json_string, size_t& position);
    static JsonValue parse_string(const std::string& json_string, size_t& position);
    static JsonValue parse_number(const std::string& json_string, size_t& position);
    static JsonValue parse_keyword(const std::string& json_string, size_t& position);

    static void skip_whitespace(const std::string& json_string, size_t& position);
    static char get_current_char(const std::string& json_string, size_t position);
    static char get_next_char(const std::string& json_string, size_t& position);

    static std::string encode_string(const std::string& str);
    static std::string decode_string(const std::string& str);
};

#define JSON_OBJECT(...) JsonValue::object().__VA_ARGS__
#define JSON_ARRAY(...) JsonValue::array().__VA_ARGS__
std::ostream& operator<<(std::ostream& os, const JsonValue& value);
std::istream& operator>>(std::istream& is, JsonValue& value);
