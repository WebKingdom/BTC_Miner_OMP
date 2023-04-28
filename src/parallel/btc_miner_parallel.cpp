#include <signal.h>

#include "../includes/utils.h"
#include "../includes/sha256_openssl.cpp"

using namespace std;

#define DEBUG 0

unsigned char running = 1;

void exit_handler(int signal) {
    printf("\nCPU: Caught signal: %d. Exiting...\n", signal);
    running = 0;
#pragma omp flush(running)
}

int main(int argc, char* argv[]) {
    // Create interrupt handling variables. Exit on a keyboard ctrl-c interrupt
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Initialize the blockchain
    const char* INIT_DATA = "[BLOCK ID|PREVIOUS DIGEST|DATA|THRESHOLD|NONCE]";
    const char* INIT_PREV_DIGEST = double_sha256(INIT_DATA);

    // Set the number of threads to use
    const size_t NUM_THREADS_MINER = omp_get_max_threads() - 2;
    const size_t NUM_VALIDATIONS = 1;
    const size_t NUM_DEVICES = omp_get_num_devices();
    // omp_set_num_threads(NUM_THREADS_MINER);
    printf("Number of CPU threads: %lu\n", NUM_THREADS_MINER);
    printf("Number of devices: %lu\n", NUM_DEVICES);

    size_t global_threshold = 0;
    size_t global_nonce = 0;
    size_t valid_nonce = 0;
    size_t validation_counter = 0;
    unsigned char verify = 0;
    unsigned char block_rejected = 0;

    // Initialize the nonce lock
    omp_lock_t lock_nonce;
    omp_lock_t lock_print;
    omp_init_lock(&lock_nonce);
    omp_init_lock(&lock_print);

    Blockchain blockchain;
    blockchain.appendBlock(INIT_PREV_DIGEST, INIT_DATA, global_threshold, global_nonce);
    global_threshold++;

    print_current_block_info(blockchain, global_nonce);

    // Start the timer
    double t_start = omp_get_wtime();
    const double T_START_GLOBAL = t_start;

#pragma omp parallel num_threads(NUM_THREADS_MINER)
    {
        // Assign a private nonce to each thread
        size_t private_nonce = 0;
#pragma omp critical
        {
            private_nonce = global_nonce;
            global_nonce++;
        }
        // Wait for all threads to assign a private nonce
#pragma omp barrier

        while (running) {
            char* data_to_hash = blockchain.getString(private_nonce);
            char* digest = double_sha256((const char*)data_to_hash);

            if (blockchain.thresholdMet((const char*)digest, global_threshold)) {
                // Found a valid nonce that provides a digest that meets the threshold requirement.
                // Only 1 thread should print the block info and update the blockchain. The other threads should verify the digest with the valid nonce.
#pragma omp single nowait
                {
                    valid_nonce = private_nonce;
                    validation_counter = 0;
                    verify = 1;
                    while (validation_counter < NUM_VALIDATIONS && !block_rejected) {
                        // Wait for all threads to verify the digest
                    }

                    if (!block_rejected) {
                        // Record time
                        omp_set_lock(&lock_print);
                        print_new_block_info(t_start, T_START_GLOBAL, digest, valid_nonce, data_to_hash);
                        omp_unset_lock(&lock_print);
                        // Append the block to the blockchain
                        blockchain.appendBlock((const char*)digest, (const char*)data_to_hash, global_threshold, valid_nonce);
                        if (global_threshold < SHA256_BITS) {
                            global_threshold++;
                        }
                        omp_set_lock(&lock_print);
                        print_current_block_info(blockchain, private_nonce);
                        omp_unset_lock(&lock_print);
                    }

                    // Reset variables
                    private_nonce = 0;
                    global_nonce = 1;
                    block_rejected = 0;
                    t_start = omp_get_wtime();
                    verify = 0;
                }
            } else {
                // Invalid nonce. Increment and try again
                omp_set_lock(&lock_nonce);
                private_nonce = global_nonce;
                global_nonce++;
                omp_unset_lock(&lock_nonce);
            }

            // The other threads should verify the digest with the valid nonce and increment the validation counter
            if (verify) {
                data_to_hash = blockchain.getString(valid_nonce);
                digest = double_sha256((const char*)data_to_hash);
                // Verify 1 thread at a time
#pragma omp critical
                {
                    if (validation_counter < NUM_VALIDATIONS) {
                        if (blockchain.thresholdMet((const char*)digest, global_threshold)) {
                            omp_set_lock(&lock_print);
                            printf("Digest accepted: \t\t%s\tNonce: %d\tTID: %d\n", digest, valid_nonce, omp_get_thread_num());
                            omp_unset_lock(&lock_print);
                            validation_counter++;
                        } else {
                            block_rejected = 1;
                            omp_set_lock(&lock_print);
                            printf("ERROR: Digest rejected: %s\tNonce: %d\tTID: %d\n", digest, valid_nonce, omp_get_thread_num());
                            omp_unset_lock(&lock_print);
                        }
                    }
                }
                while (verify) {
                    // Wait for the single thread (that found the valid nonce & digest) to print the block info and update the blockchain
                }

                // New block added, set the nonce
                omp_set_lock(&lock_nonce);
                private_nonce = global_nonce;
                global_nonce++;
                omp_unset_lock(&lock_nonce);
            }
            // free memory
            free(data_to_hash);
            free(digest);
        }
    }

    // Print then delete the blockchain
    blockchain.print();
    blockchain.~Blockchain();
    omp_destroy_lock(&lock_nonce);
    omp_destroy_lock(&lock_print);
    return 0;
}

/*
Use the following command to perform double SHA-256 in the command line:
echo -n "User Data" | openssl dgst -sha256 -binary | openssl dgst -sha256

*/
