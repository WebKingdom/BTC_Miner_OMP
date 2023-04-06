#include <omp.h>
#include <signal.h>
#include <stdlib.h>

#include <chrono>

#include "../includes/Blockchain.cpp"
#include "../includes/sha256.cpp"

using namespace std;

#define SHA256_BITS 256

bool running = true;

void exit_handler(int signal) {
    cout << "\nCaught signal: " << signal << ". Exiting..." << endl;
    running = false;
}

/**
 * @brief Prints the current (to be mined) block info to the console
 *
 * @param blockchain
 * @param nonce
 */
void print_current_block_info(Blockchain &blockchain, size_t &nonce) {
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
void print_new_block_info(auto &t_start, auto &t_start_global, string &digest, size_t &nonce, const string &data_to_hash) {
    // Record time
    auto t_end = chrono::high_resolution_clock::now();
    auto t_elapsed = chrono::duration<double>(t_end - t_start);
    auto t_global_elapsed = chrono::duration<double>(t_end - t_start_global);
    // Print the block info
    cout << "Digest: \t\t" << digest << "\tNonce: " << to_string(nonce) << "\tTID: " << omp_get_thread_num() << endl;
    cout << "Data: \t\t\t" << data_to_hash << endl;
    cout << "Block runtime: \t\t" << t_elapsed.count() << " seconds"
         << "\tTotal runtime: " << t_global_elapsed.count() << " seconds" << endl;
}

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

    size_t global_threshold = 1;
    size_t global_nonce = 0;
    size_t private_nonce = 0;
    size_t valid_nonce = 0;
    size_t validation_counter = 0;
    bool verify = false;

    Blockchain blockchain;
    blockchain.appendBlock(INIT_PREV_DIGEST, INIT_DATA, global_threshold, global_nonce);
    global_threshold++;

    print_current_block_info(blockchain, global_nonce);

    // Set the number of threads to use
    const size_t MAX_NUM_THREADS = omp_get_max_threads() - 2;
    const size_t NUM_VALIDATIONS = MAX_NUM_THREADS / 2;
    // omp_set_num_threads(MAX_NUM_THREADS);
    cout << "Number of threads: " << MAX_NUM_THREADS << endl;

    // Initialize lock for incrementing the nonce
    omp_lock_t lock;
    omp_init_lock(&lock);

    // Start the timer
    auto t_start = chrono::high_resolution_clock::now();
    auto t_start_global = t_start;

#pragma omp parallel num_threads(MAX_NUM_THREADS) private(private_nonce)
    {
        // Assign a private nonce to each thread
#pragma omp critical
        {
            cout << "Set private nonce: " << global_nonce << "\tTID: " << omp_get_thread_num() << endl;
            private_nonce = global_nonce;
            global_nonce++;
        }
        // Wait for all threads to assign a private nonce
#pragma omp barrier

        while (running) {
            string data_to_hash = blockchain.getString(private_nonce);
            string digest = double_sha256(data_to_hash);

            if (blockchain.thresholdMet(digest, global_threshold)) {
                cout << "Found valid nonce: " << private_nonce << "\tTID: " << omp_get_thread_num() << endl;
                // Found a valid nonce that provides a digest that meets the threshold requirement.
                // Only 1 thread should print the block info and update the blockchain. The other threads should verify the digest with the valid nonce.
#pragma omp single nowait
                {
                    valid_nonce = private_nonce;
                    validation_counter = 0;
                    verify = true;
                    cout << "Waiting to verify in TID: " << omp_get_thread_num() << endl;
                    while (validation_counter < NUM_VALIDATIONS) {
                        // Wait for all threads to verify the digest
                    }

                    // Record time
                    print_new_block_info(t_start, t_start_global, digest, valid_nonce, data_to_hash);
                    // Append the block to the blockchain
                    blockchain.appendBlock(digest, data_to_hash, global_threshold, valid_nonce);
                    // Reset nonce and validation counter. Increment threshold
                    private_nonce = 0;
                    global_nonce = 1;
                    if (global_threshold < SHA256_BITS) {
                        global_threshold++;
                    }

                    print_current_block_info(blockchain, private_nonce);

                    // Reset the timer
                    t_start = chrono::high_resolution_clock::now();
                    verify = false;
                }
            } else {
                // Invalid nonce. Increment and try again
                omp_set_lock(&lock);
                private_nonce = global_nonce;
                global_nonce++;
                omp_unset_lock(&lock);
            }

            // The other threads should verify the digest with the valid nonce and increment the validation counter
            if (verify) {
                data_to_hash = blockchain.getString(valid_nonce);
                digest = double_sha256(data_to_hash);
                cout << "Verifying digest: \t" << digest << "\tNonce: " << to_string(valid_nonce) << "\tTID: " << omp_get_thread_num() << endl;

                // Verify 1 thread at a time
#pragma omp critical
                {
                    if (validation_counter < NUM_VALIDATIONS) {
                        if (blockchain.thresholdMet(digest, global_threshold)) {
                            cout << "Digest accepted: \t" << digest << "\tNonce: " << to_string(valid_nonce) << "\tTID: " << omp_get_thread_num() << endl;
                            validation_counter++;
                        } else {
                            cout << "ERROR. Digest rejected: \t" << digest << "\tNonce: " << to_string(valid_nonce) << "\tTID: " << omp_get_thread_num() << endl;
                        }
                    }
                }
                cout << "Waiting for verification done in TID: " << omp_get_thread_num() << endl;
                while (verify) {
                    // Wait for the single thread (that found the valid nonce & digest) to print the block info and update the blockchain
                }

                // New block added, set the nonce
                omp_set_lock(&lock);
                private_nonce = global_nonce;
                global_nonce++;
                omp_unset_lock(&lock);
            }
        }
    }

    // Delete the blockchain and free memory
    blockchain.~Blockchain();
    omp_destroy_lock(&lock);

    return 0;
}

/*
Use the following command to perform double SHA-256 in the command line:
echo -n "User Data" | openssl dgst -sha256 -binary | openssl dgst -sha256

*/
