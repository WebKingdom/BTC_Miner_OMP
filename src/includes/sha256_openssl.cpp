#include <openssl/sha.h>
#include <string.h>

#include "defs.h"

using namespace std;

// Function for taking the SHA-256 hash of a string
char* sha256(const char* str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(digest, &sha256);

    char* buf = (char*)malloc(sizeof(char) * SHA256_DIGEST_LENGTH * 2 + 1);
    buf[SHA256_DIGEST_LENGTH * 2] = 0;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
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
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(buf + i * 2, "%02x", digest[i]);

    return buf;
}


/*
! Depreciated GPU code

// Function for taking the SHA-256 hash of a string on the GPU
#if RUN_ON_TARGET
#pragma omp declare target
#endif
char* gpu_sha256(const char* str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(digest, &sha256);

    // return digest as a char array (string) padded with 0s to 32 bytes
    char* digest_str = (char*)malloc(sizeof(char) * (SHA256_DIGEST_LENGTH + 1));
    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        digest_str[i] = digest[i];
    }
    digest_str[i] = '\0';
    return digest_str;
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif

// Function for taking the double SHA-256 hash of a string on the GPU
#if RUN_ON_TARGET
#pragma omp declare target
#endif
char* gpu_double_sha256(const char* str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(digest, &sha256);

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, digest, SHA256_DIGEST_LENGTH);
    SHA256_Final(digest, &sha256);

    // return digest as a char array (string) padded with 0s to 32 bytes
    char* digest_str = (char*)malloc(sizeof(char) * (SHA256_DIGEST_LENGTH + 1));
    int i;
    for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        digest_str[i] = digest[i];
    }
    digest_str[i] = '\0';
    return digest_str;
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif


stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }
    return ss.str();
*/
