#include <omp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../includes/Blockchain.cpp"
#include "../includes/sha256.cpp"
#include "../includes/sha256_openssl.cpp"

using namespace std;

#define DEBUG 0
#define SHA256_BITS 256

int running_cpu = 1;
int running_gpu = 1;

void exit_handler(int signal) {
    printf("\nCaught signal: %d. Exiting...\n", signal);
    running_cpu = 0;
    running_gpu = 0;
}

/**
 * @brief Prints the current (to be mined) block info to the console
 *
 * @param blockchain
 * @param nonce
 */
void print_current_block_info(Blockchain& blockchain, size_t& nonce) {
    printf("\nBLOCK ID: %lu\t\tSize: %lu\n", blockchain.getCurrentBlockId(), blockchain.getSize());
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
    printf("Digest: \t\t%s\tNonce: %lu\tTID: %d\n", digest, nonce, omp_get_thread_num());
    printf("Data: \t\t\t%s\n", data_to_hash);
    printf("Block runtime: \t\t%lf seconds\tTotal runtime: %lf seconds\n", t_elapsed, t_global_elapsed);
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
    const WORD* sha256K = InitializeK();

    // Set the number of threads to use
    const size_t MAX_NUM_THREADS = omp_get_max_threads() - 2;
    const size_t NUM_VALIDATIONS = 2;
    const size_t NUM_DEVICES = omp_get_num_devices();
    // omp_set_num_threads(MAX_NUM_THREADS);
    printf("Number of CPU threads: %lu\n", MAX_NUM_THREADS);
    printf("Number of devices: %lu\n", NUM_DEVICES);

    size_t global_threshold = 1;
    size_t global_counter = 0;
    size_t valid_nonce = 0;
    size_t validation_counter = 0;
    int verify = 0;

    Blockchain blockchain;
    blockchain.appendBlock(INIT_PREV_DIGEST, INIT_DATA, global_threshold, valid_nonce);
    global_threshold++;

    print_current_block_info(blockchain, valid_nonce);

    // Initialize lock for incrementing the nonce
    omp_lock_t lock_nonce;
    omp_lock_t lock_print;
    omp_init_lock(&lock_nonce);
    omp_init_lock(&lock_print);

    // Start the timer
    double t_start = omp_get_wtime();
    const double T_START_GLOBAL = t_start;
    const double TIME_LIMIT = 120.0;

    while (running_cpu && ((omp_get_wtime() - T_START_GLOBAL) < TIME_LIMIT)) {
        Blockchain::Block* block = blockchain.getCurrentBlock();
        size_t b_id = block->block_id;
        const char* b_prev_digest = block->prev_digest;
        const char* b_data = block->data;
        size_t b_threshold = block->threshold;
        int block_rejected = 0;

        // Start GPU threads. Block* should take 3*8B + 2*8B + 8B = 48B
#pragma omp target teams map(to: global_threshold, b_id, b_prev_digest[:strlen(b_prev_digest) + 1],          \
                             b_data[:strlen(b_data) + 1], b_threshold, sha256K[:64], blockchain, lock_nonce, \
                             lock_print) map(tofrom: global_counter, running_gpu, valid_nonce, verify)
        {
            // Assign a unique starting nonce to each team of threads
            size_t team_nonce = (MAX_SIZE_T / omp_get_num_teams()) * omp_get_team_num();
#pragma omp parallel
            {
                // Assign a private nonce to each thread
                size_t thread_nonce = team_nonce;
#pragma omp critical
                {
                    thread_nonce = team_nonce;
                    team_nonce++;
                    global_counter++;
                    // if (DEBUG)
                    //     printf("Set thread nonce: %21lu\tTeam: %5d\tTID: %5d\n", thread_nonce, omp_get_team_num(), omp_get_thread_num());
                }
                // Wait for all threads to assign a thread private nonce
#pragma omp barrier

                while (running_gpu && !verify) {
                    char* data_to_hash = blockchain.t_makeString(thread_nonce, b_id, b_prev_digest, b_data, b_threshold);
                    // if (DEBUG)
                    //     printf("String to hash: %s\tTeam: %5d\tTID: %5d\n", data_to_hash, omp_get_thread_num());

                    char* digest = gpu_double_sha256((const char*)data_to_hash, sha256K);
                    // if (DEBUG)
                    //     printf("Digest: %s\tTeam: %5d\tTID: %5d\n", digest, omp_get_thread_num());

                    if (blockchain.t_thresholdMet((const char*)digest, global_threshold)) {
                        // if (DEBUG)
                        //     printf("Found valid nonce: %21lu\tTeam: %5d\tTID: %5d\n", thread_nonce, omp_get_thread_num());
                        // Found a valid nonce that provides a digest that meets the threshold requirement.
                        // CPU will verify the digest and append the block to the blockchain
#pragma omp single nowait
                        {
                            valid_nonce = thread_nonce;
                            verify = 1;
                            running_gpu = 0;
#pragma omp flush(verify, valid_nonce, running_gpu, global_counter)
                        }
                    } else {
                        // Invalid nonce. Increment and try again
                        omp_set_lock(&lock_nonce);
                        thread_nonce = team_nonce;
                        team_nonce++;
                        global_counter++;
                        omp_unset_lock(&lock_nonce);
                    }
                    // free memory
                    free(data_to_hash);
                    free(digest);
                }  // end GPU running while loop
            }      // end parallel region
        }          // end target teams region

        // verify and append block once done with GPU section
        char* data_to_hash;
        char* digest;
        while (verify) {
            data_to_hash = blockchain.getString(valid_nonce);
            digest = double_sha256((const char*)data_to_hash);
            if (validation_counter < NUM_VALIDATIONS) {
                if (blockchain.thresholdMet((const char*)digest, global_threshold)) {
                    printf("Digest accepted: \t%s\tNonce: %lu\tTID: %d\n", digest, valid_nonce, omp_get_thread_num());
                    validation_counter++;
                } else {
                    printf("ERROR. Digest rejected: \t%s\tNonce: %lu\tTID: %d\n", digest, valid_nonce, omp_get_thread_num());
                    block_rejected = 1;
                    break;
                }
            } else {
                verify = 0;
                break;
            }
            // free memory
            free(data_to_hash);
            free(digest);
        }

        if (!block_rejected) {
            print_new_block_info(t_start, T_START_GLOBAL, digest, valid_nonce, data_to_hash);
            blockchain.appendBlock((const char*)digest, (const char*)data_to_hash, global_threshold, valid_nonce);
            if (global_threshold < SHA256_BITS) {
                global_threshold++;
            }
            print_current_block_info(blockchain, valid_nonce);
        }
        t_start = omp_get_wtime();
        // Reset variables
        running_gpu = 1;
        valid_nonce = 0;
        validation_counter = 0;

        // free memory
        free(data_to_hash);
        free(digest);
    }  // end CPU running while loop

    // Print the blockchain
    blockchain.print();

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
