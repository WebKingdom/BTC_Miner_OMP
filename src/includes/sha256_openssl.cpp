#include <openssl/sha.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "defs.h"

using namespace std;

#define SHA256_DIGEST_LENGTH 32

// Function for taking the SHA-256 hash of a string
string sha256(const char* str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(digest, &sha256);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }
    return ss.str();
}

// Function for taking the double SHA-256 hash of a string
string double_sha256(const char* str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(digest, &sha256);

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, digest, SHA256_DIGEST_LENGTH);
    SHA256_Final(digest, &sha256);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }
    return ss.str();
}

// Function for taking the SHA-256 hash of a string on the GPU
// #if RUN_ON_TARGET
// #pragma omp declare target
// #endif
// char* gpu_sha256(const char* str) {
//     unsigned char digest[SHA256_DIGEST_LENGTH];
//     SHA256_CTX sha256;
//     SHA256_Init(&sha256);
//     SHA256_Update(&sha256, str, strlen(str));
//     SHA256_Final(digest, &sha256);

//     // return digest as a char array (string) padded with 0s to 32 bytes
//     char* digest_str = (char*)malloc(sizeof(char) * (SHA256_DIGEST_LENGTH + 1));
//     int i;
//     for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
//         digest_str[i] = digest[i];
//     }
//     digest_str[i] = '\0';
//     return digest_str;
// }
// #if RUN_ON_TARGET
// #pragma omp end declare target
// #endif

// // Function for taking the double SHA-256 hash of a string on the GPU
// #if RUN_ON_TARGET
// #pragma omp declare target
// #endif
// char* gpu_double_sha256(const char* str) {
//     unsigned char digest[SHA256_DIGEST_LENGTH];
//     SHA256_CTX sha256;
//     SHA256_Init(&sha256);
//     SHA256_Update(&sha256, str, strlen(str));
//     SHA256_Final(digest, &sha256);

//     SHA256_Init(&sha256);
//     SHA256_Update(&sha256, digest, SHA256_DIGEST_LENGTH);
//     SHA256_Final(digest, &sha256);

//     // return digest as a char array (string) padded with 0s to 32 bytes
//     char* digest_str = (char*)malloc(sizeof(char) * (SHA256_DIGEST_LENGTH + 1));
//     int i;
//     for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
//         digest_str[i] = digest[i];
//     }
//     digest_str[i] = '\0';
//     return digest_str;
// }
// #if RUN_ON_TARGET
// #pragma omp end declare target
// #endif
