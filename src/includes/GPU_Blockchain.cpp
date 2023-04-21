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
// Block used to store the data for each block in the blockchain.
struct Block {
    size_t block_id;
    char *prev_digest;
    char *data;
    size_t threshold;
    size_t nonce;
    Block *next;
};

struct GPU_Blockchain {
    Block *head;
    Block *current;
    size_t num_blocks;
    size_t block_counter;

    void Delete_GPU_Blockchain();
    void removeBlock();
    void print();
    char *getString(size_t &cur_nonce);
    char *getStringCurrent(size_t &cur_nonce, Block *current);
    int isEmpty() { return (head == NULL); }
    size_t getCurrentBlockId() { return current->block_id; }
    Block *getCurrentBlock() { return current; }
    char *getPrevDigest() { return current->prev_digest; }
    size_t getSize() { return num_blocks; }
    void appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce);
    int thresholdMet(const char *digest, size_t &threshold);
    void copy_blockchain(GPU_Blockchain *other);
};

/**
 * @brief Construct a new Blockchain object
 *
 */
GPU_Blockchain* Create_GPU_Blockchain() {
    GPU_Blockchain* new_blockchain = (GPU_Blockchain*)malloc(sizeof(GPU_Blockchain));
    new_blockchain->head = NULL;
    new_blockchain->current = NULL;
    new_blockchain->num_blocks = 0;
    new_blockchain->block_counter = 0;
    return new_blockchain;
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif

/**
 * @brief Destroy the Blockchain object
 *
 */
void GPU_Blockchain::Delete_GPU_Blockchain() {
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
void GPU_Blockchain::appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce) {
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
void GPU_Blockchain::removeBlock() {
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
int GPU_Blockchain::thresholdMet(const char *digest, size_t &threshold) {
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
 * @param cur_nonce
 * @return char*
 */
char *GPU_Blockchain::getString(size_t &cur_nonce) {
    char *str = (char *)malloc(sizeof(char) * (1 + SIZE_T_STR_BYTES + 1 + strlen(current->prev_digest) + 1 + strlen(current->data) + 1 + SIZE_T_STR_BYTES + 1 + SIZE_T_STR_BYTES + 2));
    sprintf(str, "[%lu|%s|%s|%lu|%lu]", current->block_id, current->prev_digest, current->data, current->threshold, cur_nonce);
    return str;
}

/**
 * @brief Returns the string representation of the current parameter block.
 *
 * @param cur_nonce
 * @param current
 * @return char*
 */
char *GPU_Blockchain::getStringCurrent(size_t &cur_nonce, Block *current) {
    char *str = (char *)malloc(sizeof(char) * (1 + SIZE_T_STR_BYTES + 1 + strlen(current->prev_digest) + 1 + strlen(current->data) + 1 + SIZE_T_STR_BYTES + 1 + SIZE_T_STR_BYTES + 2));
    sprintf(str, "[%lu|%s|%s|%lu|%lu]", current->block_id, current->prev_digest, current->data, current->threshold, cur_nonce);
    return str;
}

/**
 * @brief Prints the blockchain.
 *
 */
void GPU_Blockchain::print() {
    Block *temp = head;
    printf("\nBlockchain with %lu blocks:\n", num_blocks);
    while (temp != NULL) {
        printf("[%lu|%s|%s|%lu|%lu]\n\n", temp->block_id, temp->prev_digest, temp->data, temp->threshold, temp->nonce);
        temp = temp->next;
    }
}

/**
 * @brief Copies the blockchain from other to this.
 *
 * @param other
 */
void GPU_Blockchain::copy_blockchain(GPU_Blockchain *other) {
    Block *temp = other->head;
    while (temp != NULL) {
        appendBlock(temp->prev_digest, temp->data, temp->threshold, temp->nonce);
        temp = temp->next;
    }
}
