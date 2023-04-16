#include <omp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

void test1() {
    // print all the teams (204 on RTX 3080) using 1 thread
#pragma omp target teams
    {
        printf("Thread = %5d, team = %5d / %5d, threads in team = %5d, max threads in team = %5d\n", omp_get_thread_num(), omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads());
    }
}

void test2() {
    size_t global_counter = 0;

    // print all the teams (204 on RTX 3080) and threads (8/team on RTX 3080) in the GPU device
#pragma omp target teams map(tofrom: global_counter)
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
        printf("Created team = %5d / %5d, threads in team = %5d, max threads in team = %5d\n", omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads());
#pragma omp parallel num_threads(2)
        {
            size_t private_counter = 0;
            // Assign a private counter to each thread in each team
#pragma omp critical
            {
                private_counter = global_counter;
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
            printf("Thread = %5d, team = %5d / %5d, threads in team = %5d, max threads in team = %5d. Count = %5ld\n", omp_get_thread_num(), omp_get_team_num(), omp_get_num_teams(), omp_get_num_threads(), omp_get_max_threads(), private_counter);
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

int main(int argc, char *argv[]) {
    // TODO test if GPU works with OpenMP

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
}
