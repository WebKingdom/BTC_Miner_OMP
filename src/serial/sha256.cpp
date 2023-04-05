#include <openssl/sha.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

#define SHA256_DIGEST_LENGTH 32

// Function for taking the SHA-256 hash of a string
string sha256(const string str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(digest, &sha256);

    stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }
    return ss.str();
}

// Function for taking the double SHA-256 hash of a string
string double_sha256(const string str) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
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
