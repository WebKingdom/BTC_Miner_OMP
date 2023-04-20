#include <omp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "../includes/Blockchain.cpp"
#include "../includes/sha256.cpp"

using namespace std;

#define DEBUG 1
#define SHA256_BITS 256

bool running = true;

void exit_handler(int signal) {
    printf("\nCaught signal: %d. Exiting...\n", signal);
    running = false;
}

/**
 * @brief Prints the current (to be mined) block info to the console
 *
 * @param blockchain
 * @param nonce
 */
#if RUN_ON_TARGET
#pragma omp declare target
#endif
void print_current_block_info(Blockchain &blockchain, size_t &nonce) {
    printf("\nBLOCK ID: %lu\t\tSize: %lu\n", blockchain.getCurrentBlockId(), blockchain.getSize());
    printf("Hash Initialization: \t%s\tNonce: %lu\tTID: %d\n", blockchain.getPrevDigest().c_str(), nonce, omp_get_thread_num());
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
void print_new_block_info(double &t_start, const double &t_start_global, char* digest, size_t &nonce, const char* data_to_hash) {
    // Record time
    double t_end = omp_get_wtime();
    double t_elapsed = t_end - t_start;
    double t_global_elapsed = t_end - t_start_global;
    // Print the block info
    printf("Digest: \t\t%s\tNonce: %lu\tTID: %d\n", digest, nonce, omp_get_thread_num());
    printf("Data: \t\t\t%s\n", data_to_hash);
    printf("Block runtime: \t\t%lf seconds\tTotal runtime: %lf seconds\n", t_elapsed, t_global_elapsed);
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif

int main(int argc, char *argv[]) {
    // Create interrupt handling variables. Exit on a keyboard ctrl-c interrupt
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Initialize the blockchain
    const string INIT_PREV_DIGEST = "0000000000000000000000000000000000000000000000000000000000000000";
    const string INIT_DATA = "This is the initial data in the 1st block";
    const size_t max_size_t = 0xFFFFFFFFFFFFFFFF;
    const WORD* sha256K = InitializeK();

    // Set the number of threads to use
    const size_t MAX_NUM_THREADS = omp_get_max_threads() - 2;
    const size_t NUM_VALIDATIONS = 2;
    const size_t NUM_DEVICES = omp_get_num_devices();
    // omp_set_num_threads(MAX_NUM_THREADS);
    cout << "Number of CPU threads: " << MAX_NUM_THREADS << endl;
    cout << "Number of devices: " << NUM_DEVICES << endl;

    size_t global_threshold = 1;
    size_t global_nonce = 0;
    size_t valid_nonce = 0;
    size_t validation_counter = 0;
    bool verify = false;

    Blockchain blockchain;
    blockchain.appendBlock(INIT_PREV_DIGEST, INIT_DATA, global_threshold, global_nonce);
    global_threshold++;

    print_current_block_info(blockchain, global_nonce);

    // Initialize lock for incrementing the nonce
    omp_lock_t lock_nonce;
    omp_lock_t lock_print;
    omp_init_lock(&lock_nonce);
    omp_init_lock(&lock_print);

    // Start the timer
    double t_start = omp_get_wtime();
    const double T_START_GLOBAL = t_start;

    // Parallelize on GPU instead of CPU
#pragma omp target teams map(to: global_nonce, validation_counter, NUM_VALIDATIONS, verify, T_START_GLOBAL, sha256K[:64]) map(tofrom: blockchain, global_threshold, valid_nonce, lock_nonce, lock_print, running, t_start)
    {
#pragma omp parallel
        {
            // Assign a private nonce to each thread
            size_t private_nonce = global_nonce;
#pragma omp critical
            {
                if (DEBUG)
                    printf("Set private nonce: %lu\tTID: %d\n", global_nonce, omp_get_thread_num());
                private_nonce = global_nonce;
                global_nonce++;
            }
            // Wait for all threads to assign a private nonce
#pragma omp barrier

            while (running) {
                const char* data_to_hash = blockchain.gpu_getString(private_nonce);
                char* digest = gpu_sha256(data_to_hash, sha256K);

                if (blockchain.thresholdMet((const char*) digest, global_threshold)) {
                    if (DEBUG) {
                        omp_set_lock(&lock_print);
                        printf("Found valid nonce: %lu\tTID: %d\n", private_nonce, omp_get_thread_num());
                        omp_unset_lock(&lock_print);
                    }
                    // Found a valid nonce that provides a digest that meets the threshold requirement.
                    // Only 1 thread should print the block info and update the blockchain. The other threads should verify the digest with the valid nonce.
#pragma omp single nowait
                    {
                        valid_nonce = private_nonce;
                        validation_counter = 0;
                        verify = true;
                        while (validation_counter < NUM_VALIDATIONS) {
                            // Wait for all threads to verify the digest
                        }

                        // Record time
                        omp_set_lock(&lock_print);
                        print_new_block_info(t_start, T_START_GLOBAL, digest, valid_nonce, data_to_hash);
                        omp_unset_lock(&lock_print);
                        // Append the block to the blockchain
                        blockchain.appendBlock(digest, data_to_hash, global_threshold, valid_nonce);
                        // Reset nonce and validation counter. Increment threshold
                        private_nonce = 0;
                        global_nonce = 1;
                        if (global_threshold < SHA256_BITS) {
                            global_threshold++;
                        }

                        omp_set_lock(&lock_print);
                        print_current_block_info(blockchain, private_nonce);
                        omp_unset_lock(&lock_print);

                        // Reset the timer
                        t_start = omp_get_wtime();
                        verify = false;
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
                    data_to_hash = blockchain.gpu_getString(valid_nonce);
                    digest = gpu_sha256(data_to_hash, sha256K);
                    // Verify 1 thread at a time
#pragma omp critical
                    {
                        if (validation_counter < NUM_VALIDATIONS) {
                            if (blockchain.thresholdMet((const char*) digest, global_threshold)) {
                                omp_set_lock(&lock_print);
                                printf("Digest accepted: \t%s\tNonce: %lu\tTID: %d\n", digest, valid_nonce, omp_get_thread_num());
                                omp_unset_lock(&lock_print);
                                validation_counter++;
                            } else {
                                omp_set_lock(&lock_print);
                                printf("ERROR. Digest rejected: \t%s\tNonce: %lu\tTID: %d\n", digest, valid_nonce, omp_get_thread_num());
                                omp_unset_lock(&lock_print);
                            }
                        }
                    }
                    while (verify) {
                        // Wait for the single thread (that found the valid nonce & digest) to print the block info and update the blockchain
                    }

                    // New block added, set the nonce
                    omp_set_lock(&lock_nonce);
                    if (DEBUG)
                        printf("Set private nonce: %lu\tTID: %d\n", global_nonce, omp_get_thread_num());
                    private_nonce = global_nonce;
                    global_nonce++;
                    omp_unset_lock(&lock_nonce);
                }
            }
        }
    }

    // Delete the blockchain and free memory
    blockchain.~Blockchain();
    omp_destroy_lock(&lock_nonce);
    omp_destroy_lock(&lock_print);

    return 0;
}

/*
Use the following command to perform double SHA-256 in the command line:
echo -n "User Data" | openssl dgst -sha256 -binary | openssl dgst -sha256

*/
