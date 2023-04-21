#include <omp.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "../includes/GPU_Blockchain.cpp"
#include "../includes/sha256.cpp"

using namespace std;

#define DEBUG 1
#define SHA256_BITS 256

int running = 1;

void exit_handler(int signal) {
    printf("\nCaught signal: %d. Exiting...\n", signal);
    running = 0;
}

/**
 * @brief Prints the current (to be mined) block info to the console
 *
 * @param blockchain
 * @param nonce
 */
void print_current_block_info(GPU_Blockchain& blockchain, size_t& nonce) {
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

#pragma omp declare target
/**
 * @brief Prints the current (to be mined) block info to the console
 *
 * @param blockchain
 * @param nonce
 */
void t_print_current_block_info(GPU_Blockchain* blockchain, size_t& nonce) {
    printf("\nBLOCK ID: %lu\t\tSize: %lu\n", blockchain->getCurrentBlockId(), blockchain->getSize());
    printf("Hash Initialization: \t%s\tNonce: %lu\tTID: %d\n", blockchain->getPrevDigest(), nonce, omp_get_thread_num());
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
void t_print_new_block_info(double& t_start, const double& t_start_global, char* digest, size_t& nonce, char* data_to_hash) {
    // Record time
    double t_end = omp_get_wtime();
    double t_elapsed = t_end - t_start;
    double t_global_elapsed = t_end - t_start_global;
    // Print the block info
    printf("Digest: \t\t%s\tNonce: %lu\tTID: %d\n", digest, nonce, omp_get_thread_num());
    printf("Data: \t\t\t%s\n", data_to_hash);
    printf("Block runtime: \t\t%lf seconds\tTotal runtime: %lf seconds\n", t_elapsed, t_global_elapsed);
}
#pragma omp end declare target

int main(int argc, char* argv[]) {
    // Create interrupt handling variables. Exit on a keyboard ctrl-c interrupt
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

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

    // fork to have main thread execute on CPU and child on GPU
    pid_t pid = fork();
    if (pid == 0) {
        // Child process, parallelize on GPU
#pragma omp target map(to: global_counter, validation_counter, NUM_VALIDATIONS, verify) map(tofrom: global_threshold, valid_nonce, running)
        {
            // Initialize the blockchain
            const char* INIT_PREV_DIGEST = "0000000000000000000000000000000000000000000000000000000000000000";
            const char* INIT_DATA = "This is the initial data in the 1st block";
            const size_t max_size_t = 0xFFFFFFFFFFFFFFFF;
            const WORD* sha256K = GPU_InitializeK();

            GPU_Blockchain* blockchain = Create_GPU_Blockchain();
            blockchain->appendBlock(INIT_PREV_DIGEST, INIT_DATA, global_threshold, global_counter);
            global_threshold++;

            t_print_current_block_info(blockchain, global_counter);

            // Initialize lock for incrementing the nonce
            omp_lock_t lock_nonce;
            omp_lock_t lock_print;
            omp_init_lock(&lock_nonce);
            omp_init_lock(&lock_print);

            // Start the timer
            double t_start = omp_get_wtime();
            const double T_START_GLOBAL = t_start;

#pragma omp teams
            {
                // Assign a unique starting nonce to each team of threads
                size_t team_nonce = (max_size_t / omp_get_num_teams()) * omp_get_team_num();
#pragma omp parallel
                {
                    // Assign a private nonce to each thread
                    size_t thread_nonce = team_nonce;
#pragma omp critical
                    {
                        thread_nonce = team_nonce;
                        team_nonce++;
                        global_counter++;
                        if (DEBUG)
                            printf("Set thread nonce: %lu\tTID: %d\n", thread_nonce, omp_get_thread_num());
                    }
                    // Wait for all threads to assign a thread private nonce
#pragma omp barrier

                    while (running) {
                        char* data_to_hash = blockchain->getString(thread_nonce);
                        if (DEBUG)
                            printf("String to hash: %s\tTID: %d\n", data_to_hash, omp_get_thread_num());
                        char* digest = gpu_double_sha256((const char*)data_to_hash, sha256K);
                        if (DEBUG)
                            printf("Digest: %s\tTID: %d\n", digest, omp_get_thread_num());

                        if (blockchain->thresholdMet((const char*)digest, global_threshold)) {
                            if (DEBUG) {
                                omp_set_lock(&lock_print);
                                printf("Found valid nonce: %lu\tTID: %d\n", thread_nonce, omp_get_thread_num());
                                omp_unset_lock(&lock_print);
                            }
                            // Found a valid nonce that provides a digest that meets the threshold requirement.
                            // Only 1 thread should print the block info and update the blockchain. The other threads should verify the digest with the valid nonce.
#pragma omp single nowait
                            {
                                valid_nonce = thread_nonce;
                                validation_counter = 0;
                                verify = 1;
                                while (validation_counter < NUM_VALIDATIONS) {
                                    // Wait for all threads to verify the digest
                                }

                                // Record time
                                omp_set_lock(&lock_print);
                                t_print_new_block_info(t_start, T_START_GLOBAL, digest, valid_nonce, data_to_hash);
                                omp_unset_lock(&lock_print);

                                // Append the block to the blockchain
                                blockchain->appendBlock((const char*)digest, (const char*)data_to_hash, global_threshold, valid_nonce);
                                // Reset nonce and validation counter. Increment threshold
                                team_nonce = (max_size_t / omp_get_num_teams()) * omp_get_team_num();
                                thread_nonce = team_nonce;
                                team_nonce++;
                                global_counter++;
                                if (global_threshold < SHA256_BITS) {
                                    global_threshold++;
                                }

                                omp_set_lock(&lock_print);
                                t_print_current_block_info(blockchain, thread_nonce);
                                omp_unset_lock(&lock_print);

                                // Reset the timer
                                t_start = omp_get_wtime();
                                verify = 0;
                            }
                        } else {
                            // Invalid nonce. Increment and try again
                            omp_set_lock(&lock_nonce);
                            thread_nonce = team_nonce;
                            team_nonce++;
                            global_counter++;
                            omp_unset_lock(&lock_nonce);
                        }

                        // The other threads should verify the digest with the valid nonce and increment the validation counter
                        if (verify) {
                            data_to_hash = blockchain->getString(valid_nonce);
                            digest = gpu_double_sha256((const char*)data_to_hash, sha256K);
                            // Verify 1 thread at a time
#pragma omp critical
                            {
                                if (validation_counter < NUM_VALIDATIONS) {
                                    if (blockchain->thresholdMet((const char*)digest, global_threshold)) {
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
                            team_nonce = (max_size_t / omp_get_num_teams()) * omp_get_team_num();
                            thread_nonce = team_nonce;
                            team_nonce++;
                            global_counter++;
                            if (DEBUG)
                                printf("Set private nonce: %lu\tTID: %d\n", thread_nonce, omp_get_thread_num());
                            omp_unset_lock(&lock_nonce);
                        }
                        // free memory
                        free(data_to_hash);
                        free(digest);
                    }

                    // Print the blockchain
                    blockchain->print();

                    // Delete the blockchain and free memory
                    blockchain->Delete_GPU_Blockchain();
                    omp_destroy_lock(&lock_nonce);
                    omp_destroy_lock(&lock_print);
                }
            }
        }
    } else if (pid > 0) {
        // Parent process
        // Wait for the child process until control-c is pressed or the child process terminates
        printf("Waiting for child process to finish\n");
        // Wait for the child process to finish
        waitpid(pid, NULL, 0);
        printf("Child process finished\n");
    } else {
        // Error forking
        printf("Error forking\n");
    }

    return 0;
}

/*
Use the following command to perform double SHA-256 in the command line:
echo -n "User Data" | openssl dgst -sha256 -binary | openssl dgst -sha256

*/
