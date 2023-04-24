#ifndef UTILS_H
#define UTILS_H

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// custom includes
#include "Blockchain.h"

/**
 * @brief Prints the current (to be mined) block info to the console
 *
 * @param blockchain
 * @param nonce
 */
void print_current_block_info(Blockchain& blockchain, size_t& nonce) {
    printf("\nBLOCK ID: %lu\t\t\t\tSize: %lu\n", blockchain.getCurrentBlockId(), blockchain.getSize());
    printf("Hash Initialization: \t%s\tNonce: %lu\tTID: %d\n", blockchain.getPrevDigest(), nonce, omp_get_thread_num());
}

/**
 * @brief Prints the newly found block info to the console
 *
 * @param t_start
 * @param t_start_global
 * @param digest
 * @param nonce
 * @param data_to_hash
 */
void print_new_block_info(double& t_start, const double& t_start_global, char* digest, size_t& nonce, char* data_to_hash) {
    // Record time
    double t_end = omp_get_wtime();
    double t_elapsed = t_end - t_start;
    double t_global_elapsed = t_end - t_start_global;
    // Print the block info
    printf("Digest: \t\t\t\t%s\tNonce: %lu\tTID: %d\n", digest, nonce, omp_get_thread_num());
    printf("Data: \t\t\t\t\t%s\n", data_to_hash);
    printf("Block runtime: \t\t\t%lf seconds\tTotal runtime: %lf seconds\n", t_elapsed, t_global_elapsed);
}

#endif
