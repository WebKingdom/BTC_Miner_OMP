#include <signal.h>

#include "../includes/utils.h"
#include "../includes/sha256.cpp"
#include "../includes/sha256_openssl.cpp"

using namespace std;

#define DEBUG 0

unsigned char running_cpu = 1;
unsigned char running_gpu = 1;

void exit_handler(int signal) {
    printf("\nCPU: Caught signal: %d. Exiting...\n", signal);
    running_cpu = 0;
    running_gpu = 0;
}

/**
 * @brief Verifies the block by checking the digest against the threshold and appends it if valid
 *
 * @param verify
 * @param block_rejected
 * @param blockchain
 * @param valid_nonce
 * @param validation_counter
 * @param global_threshold
 * @param gpu_team
 * @param gpu_tid
 * @param NUM_VALIDATIONS
 * @param t_start
 * @param T_START_GLOBAL
*/
void verify_append_block(unsigned char& verify, unsigned char& block_rejected, Blockchain& blockchain, size_t& valid_nonce, size_t& validation_counter, size_t& global_threshold, int& gpu_team, int& gpu_tid, const size_t& NUM_VALIDATIONS, double& t_start, const double& T_START_GLOBAL) {
    char* data_to_hash;
    char* digest;
    while (verify) {
        data_to_hash = blockchain.getString(valid_nonce);
        digest = double_sha256((const char*)data_to_hash);
        if (validation_counter < NUM_VALIDATIONS) {
            if (blockchain.thresholdMet((const char*)digest, global_threshold)) {
                printf("Digest accepted: \t\t%s\tNonce: %lu\tTeam: %d\tTID: %d\n", digest, valid_nonce, gpu_team, gpu_tid);
                validation_counter++;
            } else {
                printf("ERROR. Digest rejected: %s\tNonce: %lu\tTeam: %d\tTID: %d\n", digest, valid_nonce, gpu_team, gpu_tid);
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
    // free memory and update timer
    free(data_to_hash);
    free(digest);
}

int main(int argc, char* argv[]) {
    // Create interrupt handling variables. Exit on a keyboard ctrl-c interrupt
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = exit_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // Initialize the blockchain
    const WORD* sha256K = InitializeK();
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
    size_t valid_nonce = 0;
    size_t validation_counter = 0;
    int gpu_team = 0;
    int gpu_tid = 0;
    unsigned char verify = 0;
    unsigned char block_rejected = 0;

    // Initialize the nonce lock
    omp_lock_t lock_nonce;
    omp_init_lock(&lock_nonce);

    Blockchain blockchain;
    blockchain.appendBlock(INIT_PREV_DIGEST, INIT_DATA, global_threshold, valid_nonce);
    global_threshold++;

    print_current_block_info(blockchain, valid_nonce);

    // Start the timer
    const double TIME_LIMIT = 28800.0;  // 8 hours
    double t_start = omp_get_wtime();
    const double T_START_GLOBAL = t_start;

    while (running_cpu && ((omp_get_wtime() - T_START_GLOBAL) < TIME_LIMIT)) {
        Blockchain::Block* block = blockchain.getCurrentBlock();
        const size_t b_id = block->block_id;
        const char* b_prev_digest = block->prev_digest;
        const char* b_data = block->data;
        const size_t b_threshold = block->threshold;
        // Reset variables
        running_gpu = 1;
        valid_nonce = 0;
        validation_counter = 0;
        block_rejected = 0;

        // Start GPU threads
#pragma omp target teams map(to: global_threshold, b_id, b_prev_digest[:strlen(b_prev_digest)+1], b_data[:strlen(b_data)+1], b_threshold, sha256K[:64], blockchain, lock_nonce) map(tofrom: running_gpu, valid_nonce, verify, gpu_team, gpu_tid)
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
                    // printf("Init nonce: %lu\tTeam: %d\tTID: %d\n", thread_nonce, omp_get_team_num(), omp_get_thread_num());
                }
                // Wait for all threads to assign a private nonce
#pragma omp barrier

                while (running_gpu) {
                    char* data_to_hash = blockchain.t_makeString(thread_nonce, b_id, b_prev_digest, b_data, b_threshold);
                    // printf("Data to hash: %s\tTeam: %d\tTID: %d\n", data_to_hash, omp_get_team_num(), omp_get_thread_num());

                    char* digest = gpu_double_sha256((const char*)data_to_hash, sha256K);
                    // printf("Digest: %s\tTeam: %d\tTID: %d\n", digest, omp_get_team_num(), omp_get_thread_num());

                    if (blockchain.t_thresholdMet((const char*)digest, global_threshold)) {
                        // Found a valid nonce that provides a digest that meets the threshold requirement.
                        // CPU will verify the digest and append the block to the blockchain
#pragma omp single nowait
                        {
                            valid_nonce = thread_nonce;
                            verify = 1;
                            running_gpu = 0;
                            gpu_team = omp_get_team_num();
                            gpu_tid = omp_get_thread_num();
#pragma omp flush(verify, valid_nonce, running_gpu, gpu_team, gpu_tid)
                        }
                    } else {
                        // Invalid nonce. Increment and try again
                        omp_set_lock(&lock_nonce);
                        thread_nonce = team_nonce;
                        team_nonce++;
                        omp_unset_lock(&lock_nonce);
                    }
                    // free memory
                    free(data_to_hash);
                    free(digest);
                }  // end GPU running while loop
            }      // end parallel region
        }          // end target teams region

        if (running_cpu) {
            // verify and append block once done with GPU section
            verify_append_block(verify, block_rejected, blockchain, valid_nonce, validation_counter, global_threshold, gpu_team, gpu_tid, NUM_VALIDATIONS, t_start, T_START_GLOBAL);
            t_start = omp_get_wtime();
        }
    }  // end CPU running while loop

    if ((omp_get_wtime() - T_START_GLOBAL) > TIME_LIMIT) {
        printf("CPU time limit: %lf seconds reached. Exiting.\n", TIME_LIMIT);
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
