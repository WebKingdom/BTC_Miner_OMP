#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Blockchain.cpp"
#include "sha256.cpp"

using namespace std;

#define SHA256_BITS 256
#define NUM_VALIDATIONS 3

bool running = true;

void exit_handler(int signal) {
    cout << "\nCaught signal: " << signal << ". Exiting..." << endl;
    running = false;
}

int main(int argc, char *argv[]) {
    // Create interrupt handling variables. Exit on a keyboard ctrl-c interrupt
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Initialize the blockchain
    const string init_prev_digest = "0000000000000000000000000000000000000000000000000000000000000000";
    const string init_data = "This is the initial data in the 1st block";

    size_t global_threshold = 1;
    size_t global_nonce = 0;
    size_t valid_nonce = global_nonce;
    size_t validation_counter = 0;

    Blockchain blockchain;
    blockchain.appendBlock(init_prev_digest, init_data, global_threshold, global_nonce);
    global_threshold++;

    cout << "\nBLOCK ID: " << blockchain.getCurrentBlockId() << "\t\tSize: " << blockchain.getSize() << endl;
    cout << "Hash Initialization: \t" << blockchain.getPrevDigest() << "\tNonce: " << global_nonce << endl;

    auto t_start = chrono::high_resolution_clock::now();
    auto t_global_start = t_start;

    while (running) {
        const string data_to_hash = blockchain.getString(global_nonce);
        string digest = double_sha256(data_to_hash);

        if (blockchain.thresholdMet(digest, global_threshold)) {
            // Found a valid nonce that provides a digest that meets the threshold requirement.
            valid_nonce = global_nonce;
            validation_counter++;
            if (validation_counter >= NUM_VALIDATIONS) {
                auto t_end = chrono::high_resolution_clock::now();
                auto t_elapsed = chrono::duration<double>(t_end - t_start);
                auto t_global_elapsed = chrono::duration<double>(t_end - t_global_start);
                cout << "Digest: \t\t" << digest << "\tNonce: " << valid_nonce << endl;
                cout << "Data: \t\t\t" << data_to_hash << endl;
                cout << "Block runtime: \t\t" << t_elapsed.count() << " seconds"
                     << "\tTotal runtime: " << t_global_elapsed.count() << " seconds" << endl;
                blockchain.appendBlock(digest, data_to_hash, global_threshold, valid_nonce);
                // Reset nonce and validation counter. Increment threshold
                global_nonce = 0;
                validation_counter = 0;
                if (global_threshold < SHA256_BITS) {
                    global_threshold++;
                }

                // Print the upcoming block info
                cout << "\nBLOCK ID: " << blockchain.getCurrentBlockId() << "\t\tSize: " << blockchain.getSize() << endl;
                cout << "Hash Initialization: \t" << blockchain.getPrevDigest() << "\tNonce: " << global_nonce << endl;

                // Reset the timer
                t_start = chrono::high_resolution_clock::now();
            }
        } else {
            // Invalid nonce. Increment and try again
            global_nonce++;
        }
    }

    return 0;
}

/*
Use the following command to perform double SHA-256 in the command line:
echo -n "User Data" | openssl dgst -sha256 -binary | openssl dgst -sha256

*/
