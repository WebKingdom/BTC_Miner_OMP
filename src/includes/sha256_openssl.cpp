#include <openssl/sha.h>

#include "utils.h"

#pragma warning(disable : 4996)

// Function for taking the SHA-256 hash of a string
char* sha256(const char* str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(digest, &sha256);

    char* buf = (char*)malloc(sizeof(char) * SHA256_DIGEST_LENGTH * 2 + 1);
    buf[SHA256_DIGEST_LENGTH * 2] = 0;
    for (unsigned char i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(buf + i * 2, "%02x", digest[i]);

    return buf;
}

// Function for taking the double SHA-256 hash of a string
char* double_sha256(const char* str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(digest, &sha256);

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, digest, SHA256_DIGEST_LENGTH);
    SHA256_Final(digest, &sha256);

    char* buf = (char*)malloc(sizeof(char) * SHA256_DIGEST_LENGTH * 2 + 1);
    buf[SHA256_DIGEST_LENGTH * 2] = 0;
    for (unsigned char i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(buf + i * 2, "%02x", digest[i]);

    return buf;
}
