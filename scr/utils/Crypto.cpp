#include "utils/Crypto.h"
#include "utils/Logger.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <sstream>
#include <iomanip>

bool Crypto::crypto_initialized_ = false;

std::string Crypto::sha256(const std::string& data) {
    initialize_crypto();

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.length());
    SHA256_Final(hash, &sha256);

    return hex_encode(std::string(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH));
}

std::string Crypto::sha1(const std::string& data) {
    initialize_crypto();

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);

    return hex_encode(std::string(reinterpret_cast<char*>(hash), SHA_DIGEST_LENGTH));
}

std::string Crypto::hmac_sha256(const std::string& key, const std::string& data) {
    initialize_crypto();

    unsigned char* digest = HMAC(EVP_sha256(),
                                key.c_str(), key.length(),
                                reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
                                nullptr, nullptr);

    return hex_encode(std::string(reinterpret_cast<char*>(digest), SHA256_DIGEST_LENGTH));
}

std::string Crypto::encrypt_aes256(const std::string& data, const std::string& key) {
    initialize_crypto();

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return "";

    unsigned char iv[16];
    if (RAND_bytes(iv, sizeof(iv)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                          reinterpret_cast<const unsigned char*>(key.c_str()), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    int len;
    int ciphertext_len = 0;
    std::string ciphertext(data.length() + EVP_MAX_BLOCK_LENGTH, '\0');

    if (EVP_EncryptUpdate(ctx,
                         reinterpret_cast<unsigned char*>(&ciphertext[0]), &len,
                         reinterpret_cast<const unsigned char*>(data.c_str()), data.length()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx,
                           reinterpret_cast<unsigned char*>(&ciphertext[ciphertext_len]), &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    std::string result(reinterpret_cast<char*>(iv), sizeof(iv));
    result += ciphertext.substr(0, ciphertext_len);

    return base64_encode(result);
}

std::string Crypto::decrypt_aes256(const std::string& encrypted_data, const std::string& key) {
    initialize_crypto();

    std::string data = base64_decode(encrypted_data);
    if (data.length() < 16) return "";

    unsigned char iv[16];
    memcpy(iv, data.c_str(), sizeof(iv));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return "";

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                          reinterpret_cast<const unsigned char*>(key.c_str()), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    int len;
    int plaintext_len = 0;
    std::string plaintext(data.length() - sizeof(iv), '\0');

    if (EVP_DecryptUpdate(ctx,
                         reinterpret_cast<unsigned char*>(&plaintext[0]), &len,
                         reinterpret_cast<const unsigned char*>(data.c_str() + sizeof(iv)),
                         data.length() - sizeof(iv)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx,
                           reinterpret_cast<unsigned char*>(&plaintext[plaintext_len]), &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext.substr(0, plaintext_len);
}

std::string Crypto::base64_encode(const std::string& data) {
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());

    BUF_MEM* buffer_ptr;
    b64 = BIO_push(b64, mem);

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, data.c_str(), data.length());
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &buffer_ptr);

    std::string result(buffer_ptr->data, buffer_ptr->length);

    BIO_free_all(b64);
    return result;
}

std::string Crypto::base64_decode(const std::string& encoded_data) {
    if (encoded_data.empty()) return "";

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new_mem_buf(encoded_data.c_str(), encoded_data.length());

    mem = BIO_push(b64, mem);
    BIO_set_flags(mem, BIO_FLAGS_BASE64_NO_NL);

    std::string result(encoded_data.length(), '\0');
    int length = BIO_read(mem, &result[0], result.length());

    BIO_free_all(mem);

    if (length > 0) {
        result.resize(length);
        return result;
    }

    return "";
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
    initialize_crypto();

    unsigned char key[32]; // 256 bits
    if (RAND_bytes(key, sizeof(key)) != 1) {
        return "";
    }

    return std::string(reinterpret_cast<char*>(key), sizeof(key));
}

std::string Crypto::generate_random_bytes(size_t length) {
    initialize_crypto();

    std::string result(length, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char*>(&result[0]), length) != 1) {
        return "";
    }

    return result;
}

uint32_t Crypto::random_uint32() {
    initialize_crypto();

    uint32_t result;
    if (RAND_bytes(reinterpret_cast<unsigned char*>(&result), sizeof(result)) != 1) {
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

void Crypto::initialize_crypto() {
    if (!crypto_initialized_) {
        OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr);
        crypto_initialized_ = true;
    }
}

void Crypto::cleanup_crypto() {
    if (crypto_initialized_) {
        EVP_cleanup();
        crypto_initialized_ = false;
    }
}