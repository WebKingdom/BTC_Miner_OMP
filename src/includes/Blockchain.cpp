// #pragma once
#include <string.h>

#include <string>

#include "defs.h"

using namespace std;

class Blockchain {
   private:
    // Block class. Used to store the data for each block in the blockchain.
    class Block {
       public:
        size_t block_id;
        string prev_digest;
        string data;
        size_t threshold;
        size_t nonce;
        Block *next;
    };
    Block *head;
    Block *current;
    size_t num_blocks;
    size_t block_counter;

   public:
    Blockchain();
    ~Blockchain();
    bool isEmpty() { return (head == NULL); }
    void appendBlock(string prev_digest, string data, size_t threshold, size_t nonce);
    void removeBlock();
    bool thresholdMet(const char* digest, size_t &threshold);
    string getString(size_t &cur_nonce);
    char* gpu_getString(size_t &cur_nonce);
    size_t getCurrentBlockId() { return current->block_id; }
    string getPrevDigest() { return current->prev_digest; }
    size_t getSize() { return num_blocks; }
};

/**
 * @brief Construct a new Blockchain object
 *
 */
Blockchain::Blockchain() {
    head = NULL;
    current = NULL;
    num_blocks = 0;
    block_counter = 0;
}

/**
 * @brief Destroy the Blockchain object
 *
 */
Blockchain::~Blockchain() {
    while (!isEmpty()) {
        removeBlock();
    }
}

/**
 * @brief Appends a block to the end of the blockchain. Beyond the current pointer.
 *
 * @param prev_digest
 * @param data
 * @param threshold
 * @param nonce
 */
#if RUN_ON_TARGET
#pragma omp declare target
#endif
void Blockchain::appendBlock(string prev_digest, string data, size_t threshold, size_t nonce) {
    Block *new_block = new Block();
    new_block->block_id = block_counter;
    new_block->nonce = nonce;
    new_block->prev_digest = prev_digest;
    new_block->data = data;
    new_block->threshold = threshold;
    new_block->next = NULL;

    if (isEmpty()) {
        head = new_block;
        current = new_block;
    } else {
        current->next = new_block;
        current = new_block;
    }
    num_blocks++;
    block_counter++;
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif

/**
 * @brief Removes a block from the front of the blockchain.
 *
 */
void Blockchain::removeBlock() {
    if (!isEmpty()) {
        Block *temp = head;
        head = head->next;
        delete temp;
        num_blocks--;
    }
}

/**
 * @brief Checks if the digest has exactly threshold number of leading zeros.
 *
 * @param digest
 * @param threshold
 * @return true
 * @return false
 */
#if RUN_ON_TARGET
#pragma omp declare target
#endif
bool Blockchain::thresholdMet(const char* digest, size_t &threshold) {
    if (threshold >= strlen(digest)) {
        // Cannot have more leading zeros than the length of the digest.
        return false;
    }

    for (size_t i = 0; i < threshold; i++) {
        if (digest[i] != '0') {
            return false;
        }
    }

    // If the current digest has more than the threshold number of leading zeros, then it is not a valid digest.
    if (digest[threshold] == '0') {
        return false;
    }

    return true;
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif

/**
 * @brief Returns the string representation of the current block.
 *
 * @return string
 */
string Blockchain::getString(size_t &cur_nonce) {
    return "[" + to_string(current->block_id) + "|" + current->prev_digest + "|" + current->data + "|" + to_string(current->threshold) + "|" + to_string(cur_nonce) + "]";
}

/**
 * @brief Returns the string representation of the current block.
 *
 * @return string
 */
#if RUN_ON_TARGET
#pragma omp declare target
#endif
char* Blockchain::gpu_getString(size_t &cur_nonce) {
    // Assume size_t is 8 bytes (2^64 = 18,446,744,073,709,551,616). So, 6*3+2=20 bytes to fit size_t in string.
    // To be safe, use 2^128=~3.4x10^38 as max number that fits in size_t. So, use 40 bytes to fit size_t in string.
    const unsigned short size_t_bytes = 40;
    // Create string
    char *str = (char *)malloc(sizeof(char) * (1 + size_t_bytes + 1 + strlen(current->prev_digest.c_str()) + 1 + strlen(current->data.c_str()) + 1 + size_t_bytes + 1 + size_t_bytes + 2));
    sprintf(str, "[%lu|%s|%s|%lu|%lu]", current->block_id, current->prev_digest.c_str(), current->data.c_str(), current->threshold, cur_nonce);
    return str;
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif
