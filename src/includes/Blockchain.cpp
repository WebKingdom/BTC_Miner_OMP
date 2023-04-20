// #pragma once
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

using namespace std;

#if RUN_ON_TARGET
#pragma omp declare target
#endif
class Blockchain {
   private:
    // Block class. Used to store the data for each block in the blockchain.
    class Block {
       public:
        size_t block_id;
        char *prev_digest;
        char *data;
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
    void removeBlock();
    void appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce);
    int thresholdMet(const char *digest, size_t &threshold);
    char *getString(size_t &cur_nonce);
    void print();
    int isEmpty() { return (head == NULL); }
    size_t getCurrentBlockId() { return current->block_id; }
    char *getPrevDigest() { return current->prev_digest; }
    size_t getSize() { return num_blocks; }
};
#if RUN_ON_TARGET
#pragma omp end declare target
#endif

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
void Blockchain::appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce) {
    Block *new_block = (Block *)malloc(sizeof(Block));
    new_block->block_id = block_counter;
    new_block->nonce = nonce;
    // deep copy the strings
    new_block->prev_digest = (char *)malloc(sizeof(char) * (strlen(prev_digest) + 1));
    new_block->data = (char *)malloc(sizeof(char) * (strlen(data) + 1));
    strcpy(new_block->prev_digest, prev_digest);
    strcpy(new_block->data, data);
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

/**
 * @brief Removes a block from the front of the blockchain.
 *
 */
void Blockchain::removeBlock() {
    if (!isEmpty()) {
        Block *temp = head;
        head = head->next;
        free(temp->prev_digest);
        free(temp->data);
        free(temp);
        num_blocks--;
    }
}

/**
 * @brief Checks if the digest has exactly threshold number of leading zeros.
 *
 * @param digest
 * @param threshold
 * @return true - 1
 * @return false - 0
 */
int Blockchain::thresholdMet(const char *digest, size_t &threshold) {
    int valid = 1;
    if (threshold >= strlen(digest)) {
        // Cannot have more leading zeros than the length of the digest.
        valid = 0;
    }

    for (size_t i = 0; i < threshold; i++) {
        if (digest[i] != '0') {
            valid = 0;
        }
    }

    // If the current digest has more than the threshold number of leading zeros, then it is not a valid digest.
    if (digest[threshold] == '0') {
        valid = 0;
    }

    return valid;
}

/**
 * @brief Returns the string representation of the current block.
 *
 * @return char*
 */
char *Blockchain::getString(size_t &cur_nonce) {
    // Assume size_t is 8 bytes (2^64 = 18,446,744,073,709,551,616). So, 6*3+2=20 bytes to fit size_t in string.
    // To be safe, use 2^128=~3.4x10^38 as max number that fits in size_t. So, use 40 bytes to fit size_t in string.
    const unsigned short size_t_bytes = 40;
    char *str = (char *)malloc(sizeof(char) * (1 + size_t_bytes + 1 + strlen(current->prev_digest) + 1 + strlen(current->data) + 1 + size_t_bytes + 1 + size_t_bytes + 2));
    sprintf(str, "[%lu|%s|%s|%lu|%lu]", current->block_id, current->prev_digest, current->data, current->threshold, cur_nonce);
    return str;
}

/**
 * @brief Prints the blockchain.
 *
 */
void Blockchain::print() {
    Block *temp = head;
    printf("\nBlockchain with %lu blocks:\n", num_blocks);
    while (temp != NULL) {
        printf("[%lu|%s|%s|%lu|%lu]\n\n", temp->block_id, temp->prev_digest, temp->data, temp->threshold, temp->nonce);
        temp = temp->next;
    }
}
