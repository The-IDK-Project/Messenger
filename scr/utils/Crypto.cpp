#include "utils/Crypto.h"
#include "utils/Logger.h"

#include <mbedtls/sha256.h>
#include <mbedtls/sha1.h>
#include <mbedtls/md.h>
#include <mbedtls/aes.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include <mbedtls/base64.h>

#include <sstream>
#include <iomanip>
#include <memory>
#include <stdexcept>

namespace {
    struct Rng {
        mbedtls_entropy_context entropy;
        mbedtls_ctr_drbg_context ctr_drbg;

        Rng() {
            mbedtls_entropy_init(&entropy);
            mbedtls_ctr_drbg_init(&ctr_drbg);
            const char* pers = "messenger_rng";
            if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char*)pers, strlen(pers)) != 0) {
                throw std::runtime_error("Failed to seed random number generator");
            }
        }

        ~Rng() {
            mbedtls_ctr_drbg_free(&ctr_drbg);
            mbedtls_entropy_free(&entropy);
        }
    };

    Rng& get_rng() {
        static Rng rng;
        return rng;
    }
}

std::string Crypto::sha256(const std::string& data) {
    unsigned char hash[32];
    mbedtls_sha256_ret(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash, 0);
    return hex_encode(std::string(reinterpret_cast<char*>(hash), sizeof(hash)));
}

std::string Crypto::sha1(const std::string& data) {
    unsigned char hash[20];
    mbedtls_sha1_ret(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
    return hex_encode(std::string(reinterpret_cast<char*>(hash), sizeof(hash)));
}

std::string Crypto::hmac_sha256(const std::string& key, const std::string& data) {
    unsigned char digest[32];
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_hmac(md_info,
                    reinterpret_cast<const unsigned char*>(key.c_str()), key.length(),
                    reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
                    digest);
    return hex_encode(std::string(reinterpret_cast<char*>(digest), sizeof(digest)));
}

std::string Crypto::encrypt_aes256(const std::string& data, const std::string& key) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);

    unsigned char iv[16];
    if (mbedtls_ctr_drbg_random(&get_rng().ctr_drbg, iv, sizeof(iv)) != 0) {
        mbedtls_aes_free(&aes);
        return "";
    }

    mbedtls_aes_setkey_enc(&aes, reinterpret_cast<const unsigned char*>(key.c_str()), 256);

    size_t output_len = data.length() + (16 - data.length() % 16);
    std::string ciphertext(output_len, '\0');

    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, data.length(), iv,
                             reinterpret_cast<const unsigned char*>(data.c_str()),
                             reinterpret_cast<unsigned char*>(&ciphertext[0])) != 0) {
        mbedtls_aes_free(&aes);
        return "";
    }

    mbedtls_aes_free(&aes);

    std::string result(reinterpret_cast<char*>(iv), sizeof(iv));
    result += ciphertext;

    return base64_encode(result);
}

std::string Crypto::decrypt_aes256(const std::string& encrypted_data, const std::string& key) {
    std::string data = base64_decode(encrypted_data);
    if (data.length() < 16) return "";

    unsigned char iv[16];
    memcpy(iv, data.c_str(), sizeof(iv));

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, reinterpret_cast<const unsigned char*>(key.c_str()), 256);

    size_t input_len = data.length() - 16;
    std::string plaintext(input_len, '\0');

    if (mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, input_len, iv,
                             reinterpret_cast<const unsigned char*>(data.c_str() + 16),
                             reinterpret_cast<unsigned char*>(&plaintext[0])) != 0) {
        mbedtls_aes_free(&aes);
        return "";
    }

    mbedtls_aes_free(&aes);

    // Remove padding
    size_t pad_len = plaintext.back();
    if (pad_len > 0 && pad_len <= 16) {
        plaintext.resize(plaintext.length() - pad_len);
    }

    return plaintext;
}

std::string Crypto::base64_encode(const std::string& data) {
    size_t output_len = 0;
    mbedtls_base64_encode(nullptr, 0, &output_len,
                          reinterpret_cast<const unsigned char*>(data.c_str()), data.length());

    std::string result(output_len, '\0');
    mbedtls_base64_encode(reinterpret_cast<unsigned char*>(&result[0]), result.length(), &output_len,
                          reinterpret_cast<const unsigned char*>(data.c_str()), data.length());
    result.resize(output_len); // The output may be smaller than the buffer
    return result;
}

std::string Crypto::base64_decode(const std::string& encoded_data) {
    if (encoded_data.empty()) return "";

    size_t output_len = 0;
    mbedtls_base64_decode(nullptr, 0, &output_len,
                          reinterpret_cast<const unsigned char*>(encoded_data.c_str()), encoded_data.length());

    std::string result(output_len, '\0');
    size_t bytes_decoded = 0;
    if (mbedtls_base64_decode(reinterpret_cast<unsigned char*>(&result[0]), result.length(), &bytes_decoded,
                             reinterpret_cast<const unsigned char*>(encoded_data.c_str()), encoded_data.length()) != 0) {
        return "";
    }
    result.resize(bytes_decoded);
    return result;
}

std::string Crypto::hex_encode(const std::string& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : data) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

std::string Crypto::hex_decode(const std::string& encoded_data) {
    if (encoded_data.length() % 2 != 0) return "";
    std::string result;
    result.reserve(encoded_data.length() / 2);
    for (size_t i = 0; i < encoded_data.length(); i += 2) {
        std::string byte_string = encoded_data.substr(i, 2);
        char byte = static_cast<char>(std::stoul(byte_string, nullptr, 16));
        result.push_back(byte);
    }
    return result;
}

std::string Crypto::generate_aes_key() {
    return generate_random_bytes(32); // 256 bits
}

std::string Crypto::generate_random_bytes(size_t length) {
    std::string result(length, '\0');
    if (mbedtls_ctr_drbg_random(&get_rng().ctr_drbg, reinterpret_cast<unsigned char*>(&result[0]), length) != 0) {
        return "";
    }
    return result;
}

uint32_t Crypto::random_uint32() {
    uint32_t result;
    if (mbedtls_ctr_drbg_random(&get_rng().ctr_drbg, reinterpret_cast<unsigned char*>(&result), sizeof(result)) != 0) {
        return 0;
    }
    return result;
}

bool Crypto::constant_time_compare(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) {
        return false;
    }
    unsigned char result = 0;
    for (size_t i = 0; i < a.length(); ++i) {
        result |= a[i] ^ b[i];
    }
    return result == 0;
}
