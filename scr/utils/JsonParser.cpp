#include "utils/JsonParser.h"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

class JsonValue::Impl {
public:
    Type type = NULL_TYPE;
    bool bool_value = false;
    double number_value = 0.0;
    std::string string_value;
    std::vector<JsonValue> array_value;
    std::map<std::string, JsonValue> object_value;
};

JsonValue::JsonValue() : impl_(std::make_unique<Impl>()) {}

JsonValue::JsonValue(std::nullptr_t) : impl_(std::make_unique<Impl>()) {}

JsonValue::JsonValue(bool value) : impl_(std::make_unique<Impl>()) {
    impl_->type = BOOL;
    impl_->bool_value = value;
}

JsonValue::JsonValue(int value) : impl_(std::make_unique<Impl>()) {
    impl_->type = NUMBER;
    impl_->number_value = static_cast<double>(value);
}

JsonValue::JsonValue(int64_t value) : impl_(std::make_unique<Impl>()) {
    impl_->type = NUMBER;
    impl_->number_value = static_cast<double>(value);
}

JsonValue::JsonValue(double value) : impl_(std::make_unique<Impl>()) {
    impl_->type = NUMBER;
    impl_->number_value = value;
}

JsonValue::JsonValue(const std::string& value) : impl_(std::make_unique<Impl>()) {
    impl_->type = STRING;
    impl_->string_value = value;
}

JsonValue::JsonValue(const char* value) : impl_(std::make_unique<Impl>()) {
    impl_->type = STRING;
    impl_->string_value = value ? value : "";
}

JsonValue::JsonValue(const std::vector<JsonValue>& value) : impl_(std::make_unique<Impl>()) {
    impl_->type = ARRAY;
    impl_->array_value = value;
}

JsonValue::JsonValue(const std::map<std::string, JsonValue>& value) : impl_(std::make_unique<Impl>()) {
    impl_->type = OBJECT;
    impl_->object_value = value;
}

JsonValue::JsonValue(const JsonValue& other) : impl_(std::make_unique<Impl>(*other.impl_)) {}

JsonValue::JsonValue(JsonValue&& other) noexcept = default;

JsonValue& JsonValue::operator=(const JsonValue& other) {
    if (this != &other) {
        *impl_ = *other.impl_;
    }
    return *this;
}

JsonValue& JsonValue::operator=(JsonValue&& other) noexcept = default;

JsonValue::~JsonValue() = default;

JsonValue::Type JsonValue::get_type() const {
    return impl_->type;
}

bool JsonValue::is_null() const {
    return impl_->type == NULL_TYPE;
}

bool JsonValue::is_bool() const {
    return impl_->type == BOOL;
}

bool JsonValue::is_number() const {
    return impl_->type == NUMBER;
}

bool JsonValue::is_string() const {
    return impl_->type == STRING;
}

bool JsonValue::is_array() const {
    return impl_->type == ARRAY;
}

bool JsonValue::is_object() const {
    return impl_->type == OBJECT;
}

bool JsonValue::as_bool(bool default_value) const {
    return impl_->type == BOOL ? impl_->bool_value : default_value;
}

int JsonValue::as_int(int default_value) const {
    return impl_->type == NUMBER ? static_cast<int>(impl_->number_value) : default_value;
}

int64_t JsonValue::as_int64(int64_t default_value) const {
    return impl_->type == NUMBER ? static_cast<int64_t>(impl_->number_value) : default_value;
}

double JsonValue::as_double(double default_value) const {
    return impl_->type == NUMBER ? impl_->number_value : default_value;
}

std::string JsonValue::as_string(const std::string& default_value) const {
    return impl_->type == STRING ? impl_->string_value : default_value;
}

size_t JsonValue::size() const {
    if (impl_->type == ARRAY) {
        return impl_->array_value.size();
    }
    if (impl_->type == OBJECT) {
        return impl_->object_value.size();
    }
    return 0;
}

const JsonValue& JsonValue::operator[](size_t index) const {
    static JsonValue null_value;
    if (impl_->type == ARRAY && index < impl_->array_value.size()) {
        return impl_->array_value[index];
    }
    return null_value;
}

JsonValue& JsonValue::operator[](size_t index) {
    static JsonValue null_value;
    if (impl_->type != ARRAY) {
        return null_value;
    }
    if (index >= impl_->array_value.size()) {
        impl_->array_value.resize(index + 1);
    }
    return impl_->array_value[index];
}

void JsonValue::push_back(const JsonValue& value) {
    if (impl_->type == ARRAY) {
        impl_->array_value.push_back(value);
    }
}

void JsonValue::push_back(JsonValue&& value) {
    if (impl_->type == ARRAY) {
        impl_->array_value.push_back(std::move(value));
    }
}

bool JsonValue::has_key(const std::string& key) const {
    return impl_->type == OBJECT && impl_->object_value.contains(key);
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    static JsonValue null_value;
    if (impl_->type != OBJECT) {
        return null_value;
    }
    auto it = impl_->object_value.find(key);
    return it != impl_->object_value.end() ? it->second : null_value;
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (impl_->type != OBJECT) {
        impl_->type = OBJECT;
        impl_->object_value.clear();
    }
    return impl_->object_value[key];
}

void JsonValue::set(const std::string& key, const JsonValue& value) {
    (*this)[key] = value;
}

void JsonValue::set(const std::string& key, JsonValue&& value) {
    (*this)[key] = std::move(value);
}

std::vector<std::string> JsonValue::get_keys() const {
    std::vector<std::string> keys;
    if (impl_->type == OBJECT) {
        keys.reserve(impl_->object_value.size());
        for (const auto& [key, _] : impl_->object_value) {
            keys.push_back(key);
        }
    }
    return keys;
}

void JsonValue::serialize(std::ostringstream& oss, bool pretty, int indent, int current_indent) const {
    switch (impl_->type) {
        case NULL_TYPE:
            oss << "null";
            break;
        case BOOL:
            oss << (impl_->bool_value ? "true" : "false");
            break;
        case NUMBER:
            oss << impl_->number_value;
            break;
        case STRING:
            oss << std::quoted(impl_->string_value);
            break;
        case ARRAY: {
            oss << "[";
            if (pretty && !impl_->array_value.empty()) {
                oss << "\n";
            }
            for (size_t i = 0; i < impl_->array_value.size(); ++i) {
                if (pretty) {
                    oss << std::string(current_indent + indent, ' ');
                }
                impl_->array_value[i].serialize(oss, pretty, indent, current_indent + indent);
                if (i + 1 < impl_->array_value.size()) {
                    oss << ",";
                }
                if (pretty) {
                    oss << "\n";
                }
            }
            if (pretty && !impl_->array_value.empty()) {
                oss << std::string(current_indent, ' ');
            }
            oss << "]";
            break;
        }
        case OBJECT: {
            oss << "{";
            if (pretty && !impl_->object_value.empty()) {
                oss << "\n";
            }
            size_t i = 0;
            for (const auto& [key, value] : impl_->object_value) {
                if (pretty) {
                    oss << std::string(current_indent + indent, ' ');
                }
                oss << std::quoted(key) << ":";
                if (pretty) {
                    oss << " ";
                }
                value.serialize(oss, pretty, indent, current_indent + indent);
                if (++i < impl_->object_value.size()) {
                    oss << ",";
                }
                if (pretty) {
                    oss << "\n";
                }
            }
            if (pretty && !impl_->object_value.empty()) {
                oss << std::string(current_indent, ' ');
            }
            oss << "}";
            break;
        }
    }
}

std::string JsonValue::to_string(bool pretty, int indent) const {
    std::ostringstream oss;
    serialize(oss, pretty, indent, 0);
    return oss.str();
}

std::string JsonValue::to_json() const {
    return to_string(false);
}

JsonValue JsonValue::parse(const std::string& json_string) {
    return JsonParser::parse(json_string);
}

JsonValue JsonValue::array() {
    return JsonValue(std::vector<JsonValue>{});
}

JsonValue JsonValue::object() {
    return JsonValue(std::map<std::string, JsonValue>{});
}

JsonValue JsonParser::parse(const std::string& json_string) {
    size_t position = 0;
    JsonValue result = parse_value(json_string, position);
    skip_whitespace(json_string, position);
    if (position != json_string.size()) {
        throw ParseError("Unexpected trailing characters", position);
    }
    return result;
}

JsonValue JsonParser::parse_file(const std::string& filename) {
    std::ifstream input(filename);
    if (!input) {
        throw ParseError("Failed to open file", 0);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parse(buffer.str());
}

bool JsonParser::validate(const std::string& json_string) {
    try {
        parse(json_string);
        return true;
    } catch (...) {
        return false;
    }
}

std::string JsonParser::stringify(const JsonValue& value, bool pretty, int indent) {
    return value.to_string(pretty, indent);
}

bool JsonParser::write_to_file(const JsonValue& value, const std::string& filename, bool pretty) {
    std::ofstream output(filename);
    if (!output) {
        return false;
    }
    output << value.to_string(pretty);
    return output.good();
}

JsonValue JsonParser::parse_value(const std::string& json_string, size_t& position) {
    skip_whitespace(json_string, position);
    char current = get_current_char(json_string, position);

    switch (current) {
        case '{':
            return parse_object(json_string, position);
        case '[':
            return parse_array(json_string, position);
        case '"':
            return parse_string(json_string, position);
        case 't':
        case 'f':
        case 'n':
            return parse_keyword(json_string, position);
        default:
            if (current == '-' || std::isdigit(static_cast<unsigned char>(current))) {
                return parse_number(json_string, position);
            }
            throw ParseError("Unexpected character", position);
    }
}

JsonValue JsonParser::parse_object(const std::string& json_string, size_t& position) {
    JsonValue result = JsonValue::object();
    get_next_char(json_string, position);
    skip_whitespace(json_string, position);

    if (get_current_char(json_string, position) == '}') {
        get_next_char(json_string, position);
        return result;
    }

    while (true) {
        JsonValue key = parse_string(json_string, position);
        skip_whitespace(json_string, position);
        if (get_current_char(json_string, position) != ':') {
            throw ParseError("Expected ':'", position);
        }
        get_next_char(json_string, position);
        result.set(key.as_string(), parse_value(json_string, position));
        skip_whitespace(json_string, position);

        char current = get_current_char(json_string, position);
        if (current == '}') {
            get_next_char(json_string, position);
            break;
        }
        if (current != ',') {
            throw ParseError("Expected ',' or '}'", position);
        }
        get_next_char(json_string, position);
        skip_whitespace(json_string, position);
    }

    return result;
}

JsonValue JsonParser::parse_array(const std::string& json_string, size_t& position) {
    JsonValue result = JsonValue::array();
    get_next_char(json_string, position);
    skip_whitespace(json_string, position);

    if (get_current_char(json_string, position) == ']') {
        get_next_char(json_string, position);
        return result;
    }

    while (true) {
        result.push_back(parse_value(json_string, position));
        skip_whitespace(json_string, position);

        char current = get_current_char(json_string, position);
        if (current == ']') {
            get_next_char(json_string, position);
            break;
        }
        if (current != ',') {
            throw ParseError("Expected ',' or ']'", position);
        }
        get_next_char(json_string, position);
        skip_whitespace(json_string, position);
    }

    return result;
}

JsonValue JsonParser::parse_string(const std::string& json_string, size_t& position) {
    if (get_current_char(json_string, position) != '"') {
        throw ParseError("Expected string", position);
    }
    get_next_char(json_string, position);

    std::string result;
    while (position < json_string.size()) {
        char c = get_next_char(json_string, position);
        if (c == '"') {
            return JsonValue(result);
        }
        if (c == '\\') {
            char escaped = get_next_char(json_string, position);
            switch (escaped) {
                case '"':
                case '\\':
                case '/':
                    result.push_back(escaped);
                    break;
                case 'b':
                    result.push_back('\b');
                    break;
                case 'f':
                    result.push_back('\f');
                    break;
                case 'n':
                    result.push_back('\n');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'u':
                    for (int i = 0; i < 4; ++i) {
                        get_next_char(json_string, position);
                    }
                    result.push_back('?');
                    break;
                default:
                    throw ParseError("Invalid escape sequence", position);
            }
        } else {
            result.push_back(c);
        }
    }

    throw ParseError("Unterminated string", position);
}

JsonValue JsonParser::parse_number(const std::string& json_string, size_t& position) {
    size_t start = position;

    if (json_string[position] == '-') {
        ++position;
    }
    while (position < json_string.size() && std::isdigit(static_cast<unsigned char>(json_string[position]))) {
        ++position;
    }
    if (position < json_string.size() && json_string[position] == '.') {
        ++position;
        while (position < json_string.size() && std::isdigit(static_cast<unsigned char>(json_string[position]))) {
            ++position;
        }
    }
    if (position < json_string.size() && (json_string[position] == 'e' || json_string[position] == 'E')) {
        ++position;
        if (position < json_string.size() && (json_string[position] == '+' || json_string[position] == '-')) {
            ++position;
        }
        while (position < json_string.size() && std::isdigit(static_cast<unsigned char>(json_string[position]))) {
            ++position;
        }
    }

    return JsonValue(std::stod(json_string.substr(start, position - start)));
}

JsonValue JsonParser::parse_keyword(const std::string& json_string, size_t& position) {
    if (json_string.compare(position, 4, "true") == 0) {
        position += 4;
        return JsonValue(true);
    }
    if (json_string.compare(position, 5, "false") == 0) {
        position += 5;
        return JsonValue(false);
    }
    if (json_string.compare(position, 4, "null") == 0) {
        position += 4;
        return JsonValue(nullptr);
    }
    throw ParseError("Invalid keyword", position);
}

void JsonParser::skip_whitespace(const std::string& json_string, size_t& position) {
    while (position < json_string.size() && std::isspace(static_cast<unsigned char>(json_string[position]))) {
        ++position;
    }
}

char JsonParser::get_current_char(const std::string& json_string, size_t position) {
    if (position >= json_string.size()) {
        throw ParseError("Unexpected end of input", position);
    }
    return json_string[position];
}

char JsonParser::get_next_char(const std::string& json_string, size_t& position) {
    if (position >= json_string.size()) {
        throw ParseError("Unexpected end of input", position);
    }
    return json_string[position++];
}

std::string JsonParser::encode_string(const std::string& str) {
    return JsonValue(str).to_string();
}

std::string JsonParser::decode_string(const std::string& str) {
    return parse('"' + str + '"').as_string();
}

JsonParser::ParseError::ParseError(const std::string& message, size_t position)
    : std::runtime_error(message + " at position " + std::to_string(position)), position_(position) {}

size_t JsonParser::ParseError::get_position() const {
    return position_;
}

std::ostream& operator<<(std::ostream& os, const JsonValue& value) {
    os << value.to_string();
    return os;
}

std::istream& operator>>(std::istream& is, JsonValue& value) {
    std::ostringstream buffer;
    buffer << is.rdbuf();
    value = JsonValue::parse(buffer.str());
    return is;
}
