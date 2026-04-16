#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>

#include <mbedtls/sha256.h>
#include <mbedtls/sha1.h>
#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <mbedtls/rsa.h>
#include <mbedtls/pk.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/base64.h>
#include <mbedtls/error.h>

class Crypto {
public:
    static std::string sha256(const std::string& data);
    static std::string sha1(const std::string& data);
    static std::string md5(const std::string& data);
    static std::string hmac_sha256(const std::string& key, const std::string& data);

    static std::string encrypt_aes256(const std::string& data, const std::string& key);
    static std::string decrypt_aes256(const std::string& encrypted_data, const std::string& key);
    static std::string encrypt_rsa(const stds::string& data, const std::string& public_key);
    static std::string decrypt_rsa(const std::string& encrypted_data, const std::string& private_key);

    static std::string generate_aes_key();
    static std::pair<std::string, std::string> generate_rsa_keypair(int bits = 2048);
    static std::string generate_random_bytes(size_t length);

    static std::string base64_encode(const std::string& data);
    static std::string base64_decode(const std::string& encoded_data);
    static std::string hex_encode(const std::string& data);
    static std::string hex_decode(const std::string& encoded_data);

    static std::string hash_password(const std::string& password);
    static bool verify_password(const std::string& password, const std::string& hash);

    static std::string sign_data(const std::string& data, const std::string& private_key);
    static bool verify_signature(const std::string& data, const std::string& signature, const std::string& public_key);

    static uint32_t random_uint32();
    static uint64_t random_uint64();
    static std::string random_string(size_t length);

    static bool is_base64(const std::string& str);
    static bool is_hex(const std::string& str);
    static std::string sanitize_key(const std::string& key);

    static std::string pbkdf2_derive(const std::string& password, const std::string& salt, int iterations = 10000);

    static bool constant_time_compare(const std::string& a, const std::string& b);

private:
    Crypto() = delete;
};