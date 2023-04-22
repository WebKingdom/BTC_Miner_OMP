#include <omp.h>
#include <signal.h>
#include <stdlib.h>

#include <iostream>

#include "../includes/Blockchain.cpp"
#include "../includes/sha256_openssl.cpp"

using namespace std;

#define DEBUG 0
#define SHA256_BITS 256

unsigned char running = 1;

void exit_handler(int signal) {
    cout << "\nCaught signal: " << signal << ". Exiting..." << endl;
    running = 0;
}

/**
 * @brief Prints the current (to be mined) block info to the console
 *
 * @param blockchain
 * @param nonce
 */
void print_current_block_info(Blockchain& blockchain, size_t& nonce) {
    cout << "\nBLOCK ID: " << blockchain.getCurrentBlockId() << "\t\tSize: " << blockchain.getSize() << endl;
    cout << "Hash Initialization: \t" << blockchain.getPrevDigest() << "\tNonce: " << to_string(nonce) << "\tTID: " << omp_get_thread_num() << endl;
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
    cout << "Digest: \t\t" << digest << "\tNonce: " << to_string(nonce) << "\tTID: " << omp_get_thread_num() << endl;
    cout << "Data: \t\t\t" << data_to_hash << endl;
    cout << "Block runtime: \t\t" << t_elapsed << " seconds"
         << "\tTotal runtime: " << t_global_elapsed << " seconds" << endl;
}

int main(int argc, char* argv[]) {
    // Create interrupt handling variables. Exit on a keyboard ctrl-c interrupt
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Initialize the blockchain
    const char* INIT_PREV_DIGEST = "0000000000000000000000000000000000000000000000000000000000000000";
    const char* INIT_DATA = "This is the initial data in the 1st block";

    // Set the number of threads to use
    const size_t NUM_THREADS_MINER = omp_get_max_threads() - 2;
    const size_t NUM_VALIDATIONS = 1;
    // omp_set_num_threads(NUM_THREADS_MINER);
    cout << "Number of threads: " << NUM_THREADS_MINER << endl;

    size_t global_threshold = 1;
    size_t global_nonce = 0;
    size_t valid_nonce = 0;
    size_t validation_counter = 0;
    unsigned char verify = 0;

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

#pragma omp parallel num_threads(NUM_THREADS_MINER)
    {
        // Assign a private nonce to each thread
        size_t private_nonce = 0;
#pragma omp critical
        {
            private_nonce = global_nonce;
            global_nonce++;
            if (DEBUG)
                cout << "Set private nonce: " << private_nonce << "\tTID: " << omp_get_thread_num() << endl;
        }
        // Wait for all threads to assign a private nonce
#pragma omp barrier

        while (running) {
            char* data_to_hash = blockchain.getString(private_nonce);
            char* digest = double_sha256((const char*)data_to_hash);

            if (blockchain.thresholdMet((const char*)digest, global_threshold)) {
                if (DEBUG) {
                    omp_set_lock(&lock_print);
                    cout << "Found valid nonce: " << private_nonce << "\tTID: " << omp_get_thread_num() << endl;
                    omp_unset_lock(&lock_print);
                }
                // Found a valid nonce that provides a digest that meets the threshold requirement.
                // Only 1 thread should print the block info and update the blockchain. The other threads should verify the digest with the valid nonce.
#pragma omp single nowait
                {
                    valid_nonce = private_nonce;
                    validation_counter = 0;
                    verify = 1;
                    while (validation_counter < NUM_VALIDATIONS) {
                        // Wait for all threads to verify the digest
                    }

                    // Record time
                    omp_set_lock(&lock_print);
                    print_new_block_info(t_start, T_START_GLOBAL, digest, valid_nonce, data_to_hash);
                    omp_unset_lock(&lock_print);
                    // Append the block to the blockchain
                    blockchain.appendBlock((const char*)digest, (const char*)data_to_hash, global_threshold, valid_nonce);
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
                            cout << "Digest accepted: \t" << digest << "\tNonce: " << to_string(valid_nonce) << "\tTID: " << omp_get_thread_num() << endl;
                            omp_unset_lock(&lock_print);
                            validation_counter++;
                        } else {
                            omp_set_lock(&lock_print);
                            cout << "ERROR. Digest rejected: \t" << digest << "\tNonce: " << to_string(valid_nonce) << "\tTID: " << omp_get_thread_num() << endl;
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
                    cout << "Set private nonce: " << global_nonce << "\tTID: " << omp_get_thread_num() << endl;
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
