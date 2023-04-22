#include <omp.h>

// Definitions for the project
#define RUN_ON_TARGET 1
#define SHA256_DIGEST_LENGTH 32
#define MAX_SIZE_T 0xFFFFFFFFFFFFFFFF

// Assume size_t is 8 bytes (2^64 = 18,446,744,073,709,551,616). So, 6*3+2=20 bytes to fit size_t in string.
// To be safe, use 2^128=~3.4x10^38 as max number that fits in size_t. So, use 40 bytes to fit size_t in string.
#define SIZE_T_STR_BYTES 40
typedef unsigned int WORD;  // 4 Bytes

// * Define on target as well:
#pragma omp declare target
#define SHA256_DIGEST_LENGTH 32
#define MAX_SIZE_T 0xFFFFFFFFFFFFFFFF

// Assume size_t is 8 bytes (2^64 = 18,446,744,073,709,551,616). So, 6*3+2=20 bytes to fit size_t in string.
// To be safe, use 2^128=~3.4x10^38 as max number that fits in size_t. So, use 40 bytes to fit size_t in string.
#define SIZE_T_STR_BYTES 40
typedef unsigned int WORD;  // 4 Bytes
#pragma omp end declare target
