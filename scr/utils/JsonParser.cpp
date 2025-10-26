#include "utils/JsonParser.h"
#include "utils/Logger.h"
#include <sstream>
#include <iomanip>

class JsonValue::Impl {
public:
    Type type = NULL_TYPE;
    bool bool_value = false;
    double number_value = 0.0;
    std::string string_value;
    std::vector<JsonValue> array_value;
    std::map<std::string, JsonValue> object_value;
};

JsonValue::JsonValue() : impl_(std::make_unique<Impl>()) {
    impl_->type = NULL_TYPE;
}

JsonValue::JsonValue(std::nullptr_t) : impl_(std::make_unique<Impl>()) {
    impl_->type = NULL_TYPE;
}

JsonValue::JsonValue(bool value) : impl_(std::make_unique<Impl>()) {
    impl_->type = BOOL;
    impl_->bool_value = value;
}

JsonValue::JsonValue(int value) : impl_(std::make_unique<Impl>()) {
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
    impl_->string_value = value;
}

JsonValue::JsonValue(const std::vector<JsonValue>& value) : impl_(std::make_unique<Impl>()) {
    impl_->type = ARRAY;
    impl_->array_value = value;
}

JsonValue::JsonValue(const std::map<std::string, JsonValue>& value) : impl_(std::make_unique<Impl>()) {
    impl_->type = OBJECT;
    impl_->object_value = value;
}

JsonValue::JsonValue(const JsonValue& other) : impl_(std::make_unique<Impl>()) {
    *impl_ = *other.impl_;
}

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

bool JsonValue::is_null() const { return impl_->type == NULL_TYPE; }
bool JsonValue::is_bool() const { return impl_->type == BOOL; }
bool JsonValue::is_number() const { return impl_->type == NUMBER; }
bool JsonValue::is_string() const { return impl_->type == STRING; }
bool JsonValue::is_array() const { return impl_->type == ARRAY; }
bool JsonValue::is_object() const { return impl_->type == OBJECT; }

bool JsonValue::as_bool(bool default_value) const {
    if (impl_->type == BOOL) return impl_->bool_value;
    return default_value;
}

int JsonValue::as_int(int default_value) const {
    if (impl_->type == NUMBER) return static_cast<int>(impl_->number_value);
    return default_value;
}

double JsonValue::as_double(double default_value) const {
    if (impl_->type == NUMBER) return impl_->number_value;
    return default_value;
}

std::string JsonValue::as_string(const std::string& default_value) const {
    if (impl_->type == STRING) return impl_->string_value;
    return default_value;
}

size_t JsonValue::size() const {
    if (impl_->type == ARRAY) return impl_->array_value.size();
    if (impl_->type == OBJECT) return impl_->object_value.size();
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
    if (impl_->type == ARRAY) {
        if (index >= impl_->array_value.size()) {
            impl_->array_value.resize(index + 1);
        }
        return impl_->array_value[index];
    }
    return null_value;
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
    if (impl_->type == OBJECT) {
        return impl_->object_value.find(key) != impl_->object_value.end();
    }
    return false;
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    static JsonValue null_value;
    if (impl_->type == OBJECT) {
        auto it = impl_->object_value.find(key);
        if (it != impl_->object_value.end()) {
            return it->second;
        }
    }
    return null_value;
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (impl_->type != OBJECT) {
        impl_->type = OBJECT;
    }
    return impl_->object_value[key];
}

void JsonValue::set(const std::string& key, const JsonValue& value) {
    if (impl_->type != OBJECT) {
        impl_->type = OBJECT;
    }
    impl_->object_value[key] = value;
}

void JsonValue::set(const std::string& key, JsonValue&& value) {
    if (impl_->type != OBJECT) {
        impl_->type = OBJECT;
    }
    impl_->object_value[key] = std::move(value);
}

std::vector<std::string> JsonValue::get_keys() const {
    std::vector<std::string> keys;
    if (impl_->type == OBJECT) {
        for (const auto& pair : impl_->object_value) {
            keys.push_back(pair.first);
        }
    }
    return keys;
}

std::string JsonValue::to_string(bool pretty, int indent) const {
    std::ostringstream oss;
    serialize(oss, pretty, indent, 0);
    return oss.str();
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
        case ARRAY:
            oss << "[";
            if (pretty && !impl_->array_value.empty()) oss << "\n";
            for (size_t i = 0; i < impl_->array_value.size(); ++i) {
                if (pretty) oss << std::string(current_indent + indent, ' ');
                impl_->array_value[i].serialize(oss, pretty, indent, current_indent + indent);
                if (i < impl_->array_value.size() - 1) oss << ",";
                if (pretty) oss << "\n";
            }
            if (pretty && !impl_->array_value.empty()) oss << std::string(current_indent, ' ');
            oss << "]";
            break;
        case OBJECT:
            oss << "{";
            if (pretty && !impl_->object_value.empty()) oss << "\n";
            size_t i = 0;
            for (const auto& [key, value] : impl_->object_value) {
                if (pretty) oss << std::string(current_indent + indent, ' ');
                oss << std::quoted(key) << ":";
                if (pretty) oss << " ";
                value.serialize(oss, pretty, indent, current_indent + indent);
                if (++i < impl_->object_value.size()) oss << ",";
                if (pretty) oss << "\n";
            }
            if (pretty && !impl_->object_value.empty()) oss << std::string(current_indent, ' ');
            oss << "}";
            break;
    }
}

JsonValue JsonParser::parse(const std::string& json_string) {
    size_t position = 0;
    return parse_value(json_string, position);
}

JsonValue JsonParser::parse_value(const std::string& json_string, size_t& position) {
    skip_whitespace(json_string, position);

    char current = get_current_char(json_string, position);

    switch (current) {
        case '{': return parse_object(json_string, position);
        case '[': return parse_array(json_string, position);
        case '"': return parse_string(json_string, position);
        case 't': case 'f': case 'n': return parse_keyword(json_string, position);
        case '-': case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return parse_number(json_string, position);
        default:
            throw ParseError("Unexpected character", position);
    }
}

JsonValue JsonParser::parse_object(const std::string& json_string, size_t& position) {
    JsonValue result = JsonValue::object();

    get_next_char(json_string, position);
    skip_whitespace(json_string, position);

    if (get_current_char(json_string, position) == '}') {
        get_next_char(json_string, position); // Skip '}'
        return result;
    }

    while (true) {

        skip_whitespace(json_string, position);
        if (get_current_char(json_string, position) != '"') {
            throw ParseError("Expected string key", position);
        }

        JsonValue key = parse_string(json_string, position);
        std::string key_str = key.as_string();

        skip_whitespace(json_string, position);
        if (get_current_char(json_string, position) != ':') {
            throw ParseError("Expected ':'", position);
        }
        get_next_char(json_string, position);

        JsonValue value = parse_value(json_string, position);
        result.set(key_str, value);

        skip_whitespace(json_string, position);
        char current = get_current_char(json_string, position);
        if (current == '}') {
            get_next_char(json_string, position);
            break;
        } else if (current == ',') {
            get_next_char(json_string, position);
        } else {
            throw ParseError("Expected ',' or '}'", position);
        }
    }

    return result;
}

JsonValue JsonParser::parse_string(const std::string& json_string, size_t& position) {
    get_next_char(json_string, position);

    std::string result;
    bool escaped = false;

    while (position < json_string.length()) {
        char c = json_string[position++];

        if (escaped) {
            switch (c) {
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case 'u':
                    result += '?';
                    break;
                default: result += c; break;
            }
            escaped = false;
        } else {
            if (c == '"') {
                break;
            } else if (c == '\\') {
                escaped = true;
            } else {
                result += c;
            }
        }
    }

    return JsonValue(result);
}

JsonValue JsonParser::parse_number(const std::string& json_string, size_t& position) {
    size_t start = position;

    if (json_string[position] == '-') {
        position++;
    }

    while (position < json_string.length() && std::isdigit(json_string[position])) {
        position++;
    }

    if (position < json_string.length() && json_string[position] == '.') {
        position++;
        while (position < json_string.length() && std::isdigit(json_string[position])) {
            position++;
        }
    }

    if (position < json_string.length() && (json_string[position] == 'e' || json_string[position] == 'E')) {
        position++;
        if (position < json_string.length() && (json_string[position] == '+' || json_string[position] == '-')) {
            position++;
        }
        while (position < json_string.length() && std::isdigit(json_string[position])) {
            position++;
        }
    }

    std::string number_str = json_string.substr(start, position - start);
    try {
        return JsonValue(std::stod(number_str));
    } catch (...) {
        throw ParseError("Invalid number", start);
    }
}

void JsonParser::skip_whitespace(const std::string& json_string, size_t& position) {
    while (position < json_string.length() && std::isspace(json_string[position])) {
        position++;
    }
}

char JsonParser::get_current_char(const std::string& json_string, size_t position) {
    if (position >= json_string.length()) {
        throw ParseError("Unexpected end of input", position);
    }
    return json_string[position];
}

char JsonParser::get_next_char(const std::string& json_string, size_t& position) {
    if (position >= json_string.length()) {
        throw ParseError("Unexpected end of input", position);
    }
    return json_string[position++];
}

JsonParser::ParseError::ParseError(const std::string& message, size_t position)
    : std::runtime_error(message + " at position " + std::to_string(position)), position_(position) {}

size_t JsonParser::ParseError::get_position() const {
    return position_;
}


//Вот и помер дед Максим
//Да и хуй остался с ним
//Положили его в гроб
//Хуй упёрся в потолок

//Он здоровенный был мужик
//Он на хую вертел шашлык
//Хуем грядки он копал
//Хуем грядки поливал

//А соседку тётю Зину
//Он ебал через корзину
//А соседа дядю Гришу
//Хуем кинул через крышу
