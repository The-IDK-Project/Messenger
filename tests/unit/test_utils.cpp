#include <gtest/gtest.h>
#include "utils/StringUtils.h"
#include "utils/JsonParser.h"
#include "utils/Logger.h"
#include "utils/Crypto.h"

class StringUtilsTest : public ::testing::Test {};

TEST_F(StringUtilsTest, TrimOperations) {
    EXPECT_EQ(StringUtils::trim("  hello  "), "hello");
    EXPECT_EQ(StringUtils::trim_left("  hello"), "hello");
    EXPECT_EQ(StringUtils::trim_right("hello  "), "hello");
    EXPECT_EQ(StringUtils::trim("\t\nhello\t\n"), "hello");
}

TEST_F(StringUtilsTest, CaseConversion) {
    EXPECT_EQ(StringUtils::to_lower("Hello WORLD"), "hello world");
    EXPECT_EQ(StringUtils::to_upper("Hello World"), "HELLO WORLD");
    EXPECT_EQ(StringUtils::capitalize("hello"), "Hello");
}

TEST_F(StringUtilsTest, SplittingAndJoining) {
    auto parts = StringUtils::split("a,b,c", ',');
    EXPECT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[1], "b");
    EXPECT_EQ(parts[2], "c");

    EXPECT_EQ(StringUtils::join(parts, ","), "a,b,c");
    EXPECT_EQ(StringUtils::join({"x", "y", "z"}, "::"), "x::y::z");
}

TEST_F(StringUtilsTest, Searching) {
    EXPECT_TRUE(StringUtils::starts_with("hello world", "hello"));
    EXPECT_TRUE(StringUtils::ends_with("hello world", "world"));
    EXPECT_TRUE(StringUtils::contains("hello world", "lo wo"));
    EXPECT_TRUE(StringUtils::contains_ignore_case("Hello World", "hello"));
}

TEST_F(StringUtilsTest, Replacement) {
    EXPECT_EQ(StringUtils::replace("hello world", "world", "there"), "hello there");
    EXPECT_EQ(StringUtils::replace_all("a a a", "a", "b"), "b b b");
}

TEST_F(StringUtilsTest, Validation) {
    EXPECT_TRUE(StringUtils::is_number("12345"));
    EXPECT_FALSE(StringUtils::is_number("123a45"));
    EXPECT_TRUE(StringUtils::is_alpha("abc"));
    EXPECT_FALSE(StringUtils::is_alpha("abc123"));
    EXPECT_TRUE(StringUtils::is_alphanumeric("abc123"));
}

TEST_F(StringUtilsTest, Padding) {
    EXPECT_EQ(StringUtils::pad_left("5", 3, '0'), "005");
    EXPECT_EQ(StringUtils::pad_right("5", 3, '0'), "500");
    EXPECT_EQ(StringUtils::pad_center("5", 5, '-'), "--5--");
}

class JsonParserTest : public ::testing::Test {};

TEST_F(JsonParserTest, BasicTypes) {

    JsonValue null_val = JsonValue::parse("null");
    EXPECT_TRUE(null_val.is_null());

    JsonValue bool_val = JsonValue::parse("true");
    EXPECT_TRUE(bool_val.is_bool());
    EXPECT_TRUE(bool_val.as_bool());

    JsonValue num_val = JsonValue::parse("42.5");
    EXPECT_TRUE(num_val.is_number());
    EXPECT_DOUBLE_EQ(num_val.as_double(), 42.5);
    EXPECT_EQ(num_val.as_int(), 42);

    JsonValue str_val = JsonValue::parse("\"hello world\"");
    EXPECT_TRUE(str_val.is_string());
    EXPECT_EQ(str_val.as_string(), "hello world");
}

TEST_F(JsonParserTest, Arrays) {
    JsonValue array = JsonValue::parse("[1, 2, 3, \"four\"]");
    EXPECT_TRUE(array.is_array());
    EXPECT_EQ(array.size(), 4);
    EXPECT_EQ(array[0].as_int(), 1);
    EXPECT_EQ(array[3].as_string(), "four");
}

TEST_F(JsonParserTest, Objects) {
    JsonValue obj = JsonValue::parse("{\"name\": \"John\", \"age\": 30}");
    EXPECT_TRUE(obj.is_object());
    EXPECT_TRUE(obj.has_key("name"));
    EXPECT_TRUE(obj.has_key("age"));
    EXPECT_EQ(obj["name"].as_string(), "John");
    EXPECT_EQ(obj["age"].as_int(), 30);
}

TEST_F(JsonParserTest, Serialization) {
    JsonValue obj = JsonValue::object();
    obj["name"] = "Test";
    obj["values"] = JsonValue::array();
    obj["values"].push_back(1);
    obj["values"].push_back(2);

    std::string json_str = obj.to_string();
    EXPECT_TRUE(json_str.find("\"name\"") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"Test\"") != std::string::npos);
}

TEST_F(JsonParserTest, ErrorHandling) {
    EXPECT_THROW(JsonValue::parse("{invalid}"), JsonParser::ParseError);
    EXPECT_THROW(JsonValue::parse("[1, 2,"), JsonParser::ParseError);
}

class CryptoTest : public ::testing::Test {};

TEST_F(CryptoTest, Hashing) {
    std::string data = "hello world";
    std::string hash = Crypto::sha256(data);

    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.length(), 64);

    EXPECT_EQ(Crypto::sha256(data), hash);

    EXPECT_NE(Crypto::sha256("different"), hash);
}

TEST_F(CryptoTest, Base64Encoding) {
    std::string original = "Hello, World!";
    std::string encoded = Crypto::base64_encode(original);
    std::string decoded = Crypto::base64_decode(encoded);

    EXPECT_EQ(decoded, original);
    EXPECT_FALSE(encoded.empty());
}

TEST_F(CryptoTest, HexEncoding) {
    std::string data = "test";
    std::string hex = Crypto::hex_encode(data);
    std::string decoded = Crypto::hex_decode(hex);

    EXPECT_EQ(decoded, data);
    EXPECT_FALSE(hex.empty());
}

TEST_F(CryptoTest, RandomGeneration) {
    auto random1 = Crypto::generate_random_bytes(16);
    auto random2 = Crypto::generate_random_bytes(16);

    EXPECT_EQ(random1.length(), 16);
    EXPECT_EQ(random2.length(), 16);
    EXPECT_NE(random1, random2);
}

TEST_F(CryptoTest, ConstantTimeCompare) {
    std::string a = "same string";
    std::string b = "same string";
    std::string c = "different";

    EXPECT_TRUE(Crypto::constant_time_compare(a, b));
    EXPECT_FALSE(Crypto::constant_time_compare(a, c));
    EXPECT_FALSE(Crypto::constant_time_compare(a, "shorter"));
}