#include <signal.h>

#include "../includes/utils.h"
#include "../includes/sha256_openssl.cpp"

using namespace std;

#define NUM_VALIDATIONS 1

unsigned char running = 1;

void exit_handler(int signal) {
    printf("\nCPU: Caught signal: %d. Exiting...\n", signal);
    running = 0;
}

int main(int argc, char *argv[]) {
    // Create interrupt handling variables. Exit on a keyboard ctrl-c interrupt
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Initialize the blockchain
    const char *INIT_DATA = "[BLOCK ID|PREVIOUS DIGEST|DATA|THRESHOLD|NONCE]";
    const char *INIT_PREV_DIGEST = double_sha256(INIT_DATA);

    size_t global_threshold = 0;
    size_t global_nonce = 0;
    size_t valid_nonce = 0;
    size_t validation_counter = 0;

    Blockchain blockchain;
    blockchain.appendBlock(INIT_PREV_DIGEST, INIT_DATA, global_threshold, global_nonce);
    global_threshold++;

    print_current_block_info(blockchain, global_nonce);

    // Start the timer
    double t_start = omp_get_wtime();
    const double T_START_GLOBAL = t_start;

    while (running) {
        char *data_to_hash = blockchain.getString(global_nonce);
        char *digest = double_sha256((const char *)data_to_hash);

        if (blockchain.thresholdMet((const char *)digest, global_threshold)) {
            // Found a valid nonce that provides a digest that meets the threshold requirement.
            valid_nonce = global_nonce;
            validation_counter++;
            if (validation_counter >= NUM_VALIDATIONS) {
                // Record time
                print_new_block_info(t_start, T_START_GLOBAL, digest, valid_nonce, data_to_hash);
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
                t_start = omp_get_wtime();
            }
        } else {
            // Invalid nonce. Increment and try again
            global_nonce++;
        }
        // free memory
        free(data_to_hash);
        free(digest);
    }

    // Print then delete the blockchain
    blockchain.print();
    blockchain.~Blockchain();
    return 0;
}

/*
Use the following command to perform double SHA-256 in the command line:
echo -n "User Data" | openssl dgst -sha256 -binary | openssl dgst -sha256

*/
