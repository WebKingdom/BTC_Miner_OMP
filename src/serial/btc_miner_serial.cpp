#include <signal.h>
#include <stdlib.h>

#include <chrono>
#include <iostream>

#include "../includes/Blockchain.cpp"
#include "../includes/sha256_openssl.cpp"

using namespace std;

#define SHA256_BITS 256
#define NUM_VALIDATIONS 2

int running = 1;

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
void print_current_block_info(Blockchain &blockchain, size_t &nonce) {
    cout << "\nBLOCK ID: " << blockchain.getCurrentBlockId() << "\t\tSize: " << blockchain.getSize() << endl;
    cout << "Hash Initialization: \t" << blockchain.getPrevDigest() << "\tNonce: " << to_string(nonce) << endl;
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
void print_new_block_info(auto &t_start, auto &t_start_global, char* digest, size_t &nonce, char* data_to_hash) {
    // Record time
    auto t_end = chrono::high_resolution_clock::now();
    auto t_elapsed = chrono::duration<double>(t_end - t_start);
    auto t_global_elapsed = chrono::duration<double>(t_end - t_start_global);
    // Print the block info
    cout << "Digest: \t\t" << digest << "\tNonce: " << to_string(nonce) << endl;
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
    const char *init_prev_digest = "0000000000000000000000000000000000000000000000000000000000000000";
    const char *init_data = "This is the initial data in the 1st block";

    size_t global_threshold = 1;
    size_t global_nonce = 0;
    size_t valid_nonce = 0;
    size_t validation_counter = 0;

    Blockchain blockchain;
    blockchain.appendBlock(init_prev_digest, init_data, global_threshold, global_nonce);
    global_threshold++;

    print_current_block_info(blockchain, global_nonce);

    // Start the timer
    auto t_start = chrono::high_resolution_clock::now();
    auto t_start_global = t_start;

    while (running) {
        char *data_to_hash = blockchain.getString(global_nonce);
        char *digest = double_sha256((const char *)data_to_hash);

        if (blockchain.thresholdMet((const char *)digest, global_threshold)) {
            // Found a valid nonce that provides a digest that meets the threshold requirement.
            valid_nonce = global_nonce;
            validation_counter++;
            if (validation_counter >= NUM_VALIDATIONS) {
                // Record time
                print_new_block_info(t_start, t_start_global, digest, valid_nonce, data_to_hash);
                // Append the block to the blockchain
                blockchain.appendBlock((const char *)digest, (const char *)data_to_hash, global_threshold, valid_nonce);
                // Reset nonce and validation counter. Increment threshold
                global_nonce = 0;
                validation_counter = 0;
                if (global_threshold < SHA256_BITS) {
                    global_threshold++;
                }

                print_current_block_info(blockchain, global_nonce);

                // Reset the timer
                t_start = chrono::high_resolution_clock::now();
            }
        } else {
            // Invalid nonce. Increment and try again
            global_nonce++;
        }
        // free memory
        free(data_to_hash);
        free(digest);
    }

    // Print the blockchain
    blockchain.print();

    // Delete the blockchain and free memory
    blockchain.~Blockchain();

    return 0;
}

/*
Use the following command to perform double SHA-256 in the command line:
echo -n "User Data" | openssl dgst -sha256 -binary | openssl dgst -sha256

*/
