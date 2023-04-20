#include <omp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

#include "../includes/defs.h"
#include "../includes/sha256.cpp"

using namespace std;

void test1() {
    size_t global_counter = 0;
    const size_t max_size_t = 0xFFFFFFFFFFFFFFFF;
    WORD* sha256K = InitializeK();

    // print all the teams (204 on RTX 3080) and threads (8/team on RTX 3080) in the GPU device
#pragma omp target teams map(to: sha256K[:64]) map(tofrom: global_counter)
    {
        /**
         * The TEAMS construct creates a league of one-thread teams where the thread of each team executes
         * concurrently and is in its own contention group. The number of teams created is implementation defined,
         * but is no more than num_teams if specified in the clause. The maximum number of threads participating in
         * the contention group that each team initiates is implementation defined as well, unless thread_limit is
         * specified in the clause. Threads in a team can synchronize but no synchronization among teams. The TEAMS
         * construct must be contained in a TARGET construct, without any other directives, statements or declarations
         * in between.
         *
         * A contention group is the set of all threads that are descendants of an initial thread. An initial thread
         * is never a descendant of another initial thread.
         *
         */

        size_t team_counter = (max_size_t / omp_get_num_teams()) * omp_get_team_num();
        printf("Created team = %5d / %5d, threads in team = %5d, max threads in team = %5d, team counter = %21lu\n", omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads(), team_counter);
#pragma omp parallel
        {
            // Assign a private counter to each thread in each team
            size_t thread_counter = team_counter;
#pragma omp critical
            {
                thread_counter = team_counter;
                team_counter++;
                global_counter++;
            }
#pragma omp barrier

            /**
             * To further create threads within each team and distribute loop iterations across threads, we will use
             * the PARALLEL FOR/DO constructs.
             *
             * TEAMS DISTRIBUTE construct
             *      Coarse-grained parallelism
             *      Spawns multiple single-thread teams
             *      No synchronization of threads in different teams
             * PARALLEL FOR/DO construct
             *      Fine-grained parallelism
             *      Spawns many threads in one team
             *      Threads can synchronize in a team
             *
             */

            // hash the data
            char* data = (char *)malloc(sizeof(char) * 40);
            char* sha256K_str = (char *)malloc(sizeof(WORD) * 65 * 2);
            // max index should be 4*65*2=520
            sha256K_str[519] = 0;
            for (int i = 0; i < 64; i++) {
                sprintf(sha256K_str + (i * 8), "%08x", sha256K[i]);
            }
            sprintf(data, "%lu", thread_counter);
            char* digest = gpu_sha256((const char*) data, sha256K);

            printf("Thread = %5d, team = %5d / %5d, threads in team = %5d, max threads in team = %5d. Thread counter = %21lu, data = %s, digest = %s\n", omp_get_thread_num(), omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads(), thread_counter, data, digest);
        }
    }

    printf("Global counter = %ld\n", global_counter);
}

void test2() {
    size_t global_counter = 0;
    const size_t max_size_t = 0xFFFFFFFFFFFFFFFF;
    WORD* sha256K = InitializeK();

    // print all the teams (204 on RTX 3080) and threads (8/team on RTX 3080) in the GPU device
#pragma omp target teams map(to: sha256K[:64]) map(tofrom: global_counter)
    {
        /**
         * The TEAMS construct creates a league of one-thread teams where the thread of each team executes
         * concurrently and is in its own contention group. The number of teams created is implementation defined,
         * but is no more than num_teams if specified in the clause. The maximum number of threads participating in
         * the contention group that each team initiates is implementation defined as well, unless thread_limit is
         * specified in the clause. Threads in a team can synchronize but no synchronization among teams. The TEAMS
         * construct must be contained in a TARGET construct, without any other directives, statements or declarations
         * in between.
         *
         * A contention group is the set of all threads that are descendants of an initial thread. An initial thread
         * is never a descendant of another initial thread.
         *
         */

        size_t team_counter = (max_size_t / omp_get_num_teams()) * omp_get_team_num();
        printf("Created team = %5d / %5d, threads in team = %5d, max threads in team = %5d, team counter = %21lu\n", omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads(), team_counter);
#pragma omp parallel
        {
            // Assign a private counter to each thread in each team
            size_t thread_counter = team_counter;
#pragma omp critical
            {
                thread_counter = team_counter;
                team_counter++;
                global_counter++;
            }
#pragma omp barrier

            /**
             * To further create threads within each team and distribute loop iterations across threads, we will use
             * the PARALLEL FOR/DO constructs.
             *
             * TEAMS DISTRIBUTE construct
             *      Coarse-grained parallelism
             *      Spawns multiple single-thread teams
             *      No synchronization of threads in different teams
             * PARALLEL FOR/DO construct
             *      Fine-grained parallelism
             *      Spawns many threads in one team
             *      Threads can synchronize in a team
             *
             */

            // hash the data
            char* data = (char *)malloc(sizeof(char) * 40);
            char* sha256K_str = (char *)malloc(sizeof(WORD) * 65 * 2);
            // max index should be 4*65*2=520
            sha256K_str[519] = 0;
            for (int i = 0; i < 64; i++) {
                sprintf(sha256K_str + (i * 8), "%08x", sha256K[i]);
            }
            sprintf(data, "%lu", thread_counter);

            char* digest = gpu_double_sha256((const char*) data, sha256K);
            printf("Thread = %5d, team = %5d / %5d, threads in team = %5d, max threads in team = %5d. Thread counter = %21lu, data = %s, digest = %s\n", omp_get_thread_num(), omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads(), thread_counter, data, digest);
        }
    }

    printf("Global counter = %ld\n", global_counter);
}

void test3() {
    // prints all the threads within 1 team (8 threads / team on RTX 3080)
#pragma omp target parallel
    {
        printf("Thread = %5d, team = %5d / %5d, threads in team = %5d, max threads in team = %5d\n", omp_get_thread_num(), omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads());
    }
}

void test4() {
    // prints all the threads within 1 team (8 threads / team on RTX 3080)
#pragma omp target teams
    {
#pragma omp parallel
        {
            printf("Thread = %5d, team = %5d / %5d, threads in team = %5d, max threads in team = %5d\n", omp_get_thread_num(), omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads());
        }
    }
}

void test_strings() {
    // Assume size_t is 8 bytes (2^64 = 18,446,744,073,709,551,616). So, 6*3+2=20 bytes to fit size_t in string.
    // To be safe, use 2^128=~3.4x10^38 as max number that fits in size_t. So, use 40 bytes to fit size_t in string.
    const unsigned short size_t_bytes = 40;
    size_t block_id = 1;
    string prev_digest = "0123456789";
    string data = "Data in 1st block";
    size_t threshold = 2;
    size_t cur_nonce = 0;

    // print the sizes of each variable
    printf("Size of block_id = %lu\n", sizeof(block_id));
    printf("Size of prev_digest = %lu\n", sizeof(prev_digest.c_str()));
    printf("Strlen of prev_digest = %lu\n", strlen(prev_digest.c_str()));
    printf("Size of data = %lu\n", sizeof(data.c_str()));
    printf("Strlen of data = %lu\n", strlen(data.c_str()));
    printf("Size of threshold = %lu\n", sizeof(threshold));
    printf("Size of cur_nonce = %lu\n", sizeof(cur_nonce));

    // allocate memory for the string based on the size of the block_id, prev_digest, data, threshold, and nonce.
    char *str = (char *)malloc(sizeof(char) * (1 + size_t_bytes + 1 + strlen(prev_digest.c_str()) + 1 + strlen(data.c_str()) + 1 + size_t_bytes + 1 + size_t_bytes + 2));
    // create the string
    sprintf(str, "[%lu|%s|%s|%lu|%lu]", block_id, prev_digest.c_str(), data.c_str(), threshold, cur_nonce);

    // print the string
    printf("Size of str = %lu\n", sizeof(str));
    printf("Strlen of str = %lu\n", strlen(str));
    printf("String = %s\n", str);

    // modify the string
    str[strlen(str)] = 'A';
    str[strlen(str)] = 'B';
    printf("Strlen of str = %lu\n", strlen(str));
    printf("String = %s\n", str);
}

int main(int argc, char *argv[]) {
    int default_device = omp_get_default_device();
    int num_devices = omp_get_num_devices();
    printf("Default device: %d of %d devices in total\n", default_device, num_devices);

    // Set the default device to the first GPU device
    // omp_set_default_device(1);
    // printf("Default device now: %d of %d devices in total\n", omp_get_default_device(), omp_get_num_devices());

    // get number of threads in the default device
    int num_threads = omp_get_max_threads();
    printf("Number of threads in the default device: %d\n", num_threads);

    // get number of teams in the default device
    int num_teams = omp_get_num_teams();
    printf("Number of teams in the default device: %d\n", num_teams);

    // test1();
    // printf("--------------------------------------------\n");
    test2();
    // printf("--------------------------------------------\n");
    // test3();
    // printf("--------------------------------------------\n");
    // test4();

    // printf("--------------------------------------------\n");
    // test_strings();
}
