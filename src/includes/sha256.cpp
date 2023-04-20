#include <string.h>

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "defs.h"
#include "Sha.h"

#if RUN_ON_TARGET
#pragma omp declare target
#endif
#define BLOCKSIZE 64
#define ROTRIGHT(a, b) (((a) >> (b)) | ((a) << (32 - (b))))
#define CH(a, b, c) (((a) & (b)) ^ (~(a) & (c)))
#define MAJ(a, b, c) (((a) & (b)) ^ ((a) & (c)) ^ ((b) & (c)))
#define SIGMA0(a) (ROTRIGHT(a, 7) ^ ROTRIGHT(a, 18) ^ ((a) >> 3))
#define SIGMA1(a) (ROTRIGHT(a, 17) ^ ROTRIGHT(a, 19) ^ ((a) >> 10))
#define SUM0(a) (ROTRIGHT(a, 2) ^ ROTRIGHT(a, 13) ^ ROTRIGHT(a, 22))
#define SUM1(a) (ROTRIGHT(a, 6) ^ ROTRIGHT(a, 11) ^ ROTRIGHT(a, 25))
#define WORDTOCHAR(a, str)                         \
    {                                              \
        *((str) + 3) = (unsigned char)((a));       \
        *((str) + 2) = (unsigned char)((a) >> 8);  \
        *((str) + 1) = (unsigned char)((a) >> 16); \
        *((str) + 0) = (unsigned char)((a) >> 24); \
    }
#define CHARTOWORD(str, x)                                                                                                         \
    {                                                                                                                              \
        *(x) = ((WORD) * ((str) + 3)) | ((WORD) * ((str) + 2) << 8) | ((WORD) * ((str) + 1) << 16) | ((WORD) * ((str) + 0) << 24); \
    }
#if RUN_ON_TARGET
#pragma omp end declare target
#endif

WORD* InitializeK() {
    WORD* k = (WORD *)malloc(sizeof(WORD) * 64);
    k[0] = 0x428a2f98;
    k[1] = 0x71374491;
    k[2] = 0xb5c0fbcf;
    k[3] = 0xe9b5dba5;
    k[4] = 0x3956c25b;
    k[5] = 0x59f111f1;
    k[6] = 0x923f82a4;
    k[7] = 0xab1c5ed5;
    k[8] = 0xd807aa98;
    k[9] = 0x12835b01;
    k[10] = 0x243185be;
    k[11] = 0x550c7dc3;
    k[12] = 0x72be5d74;
    k[13] = 0x80deb1fe;
    k[14] = 0x9bdc06a7;
    k[15] = 0xc19bf174;
    k[16] = 0xe49b69c1;
    k[17] = 0xefbe4786;
    k[18] = 0x0fc19dc6;
    k[19] = 0x240ca1cc;
    k[20] = 0x2de92c6f;
    k[21] = 0x4a7484aa;
    k[22] = 0x5cb0a9dc;
    k[23] = 0x76f988da;
    k[24] = 0x983e5152;
    k[25] = 0xa831c66d;
    k[26] = 0xb00327c8;
    k[27] = 0xbf597fc7;
    k[28] = 0xc6e00bf3;
    k[29] = 0xd5a79147;
    k[30] = 0x06ca6351;
    k[31] = 0x14292967;
    k[32] = 0x27b70a85;
    k[33] = 0x2e1b2138;
    k[34] = 0x4d2c6dfc;
    k[35] = 0x53380d13;
    k[36] = 0x650a7354;
    k[37] = 0x766a0abb;
    k[38] = 0x81c2c92e;
    k[39] = 0x92722c85;
    k[40] = 0xa2bfe8a1;
    k[41] = 0xa81a664b;
    k[42] = 0xc24b8b70;
    k[43] = 0xc76c51a3;
    k[44] = 0xd192e819;
    k[45] = 0xd6990624;
    k[46] = 0xf40e3585;
    k[47] = 0x106aa070;
    k[48] = 0x19a4c116;
    k[49] = 0x1e376c08;
    k[50] = 0x2748774c;
    k[51] = 0x34b0bcb5;
    k[52] = 0x391c0cb3;
    k[53] = 0x4ed8aa4a;
    k[54] = 0x5b9cca4f;
    k[55] = 0x682e6ff3;
    k[56] = 0x748f82ee;
    k[57] = 0x78a5636f;
    k[58] = 0x84c87814;
    k[59] = 0x8cc70208;
    k[60] = 0x90befffa;
    k[61] = 0xa4506ceb;
    k[62] = 0xbef9a3f7;
    k[63] = 0xc67178f2;
    return k;
}

#if RUN_ON_TARGET
#pragma omp declare target
#endif
void WordExtend(WORD* words) {
    for (size_t i = 16; i < 64; i++)
        words[i] = SIGMA1(words[i - 2]) + words[i - 7] + SIGMA0(words[i - 15]) + words[i - 16];
}

void WordCompress(WORD* abcdefgh, const WORD* sha256K, WORD* expandedWords) {
    WORD temp1, temp2;
    for (int i = 0; i < 64; i++) {
        temp1 = abcdefgh[7] + SUM1(abcdefgh[4]) + CH(abcdefgh[4], abcdefgh[5], abcdefgh[6]) + sha256K[i] + expandedWords[i];
        temp2 = SUM0(abcdefgh[0]) + MAJ(abcdefgh[0], abcdefgh[1], abcdefgh[2]);
        abcdefgh[7] = abcdefgh[6];
        abcdefgh[6] = abcdefgh[5];
        abcdefgh[5] = abcdefgh[4];
        abcdefgh[4] = abcdefgh[3] + temp1;
        abcdefgh[3] = abcdefgh[2];
        abcdefgh[2] = abcdefgh[1];
        abcdefgh[1] = abcdefgh[0];
        abcdefgh[0] = temp1 + temp2;
    }
}

void Transform(const unsigned char* message, WORD blockNum, const WORD* sha256K, WORD* sha256H) {
    WORD expandedWords[64];
    WORD abcdefgh[8];

    const unsigned char* tempBlock;
    int i, j;
    for (i = 0; i < (int)blockNum; i++) {
        tempBlock = message + (i << 6);
        for (j = 0; j < 16; j++) {
            CHARTOWORD(&tempBlock[j << 2], &expandedWords[j]);
        }
        // kelime genisletme algoritmasi
        WordExtend(expandedWords);

        for (j = 0; j < 8; j++) {
            abcdefgh[j] = sha256H[j];
        }
        // word compress algoritmasi
        WordCompress(abcdefgh, sha256K, expandedWords);

        for (j = 0; j < 8; j++) {
            sha256H[j] += abcdefgh[j];
        }
    }
}

void Update(const unsigned char* message, WORD len, const WORD* sha256K, WORD* sha256H, unsigned char* msgBlock, WORD& msgTotalLen,
            WORD& msgLen) {
    WORD blockNum, remLen, tempLen;
    const unsigned char* shiftedMsg;
    tempLen = BLOCKSIZE - msgLen;
    remLen = len < tempLen ? len : tempLen;

    memcpy(&msgBlock[msgLen], message, remLen);
    if (msgLen + len < BLOCKSIZE) {
        msgLen += len;
    } else {
        len -= remLen;
        blockNum = len / BLOCKSIZE;
        shiftedMsg = message + remLen;

        Transform(msgBlock, 1, sha256K, sha256H);
        Transform(shiftedMsg, blockNum, sha256K, sha256H);
        remLen = len % BLOCKSIZE;
        memcpy(msgBlock, &shiftedMsg[blockNum << 6], remLen);
        msgLen = remLen;
        msgTotalLen += (blockNum + 1) << 6;
    }
}

void Final(unsigned char* digest, const WORD* sha256K, WORD* sha256H, unsigned char* msgBlock, WORD& msgTotalLen, WORD& msgLen) {
    WORD blockNum, tempLen, lenB;

    blockNum = (1 + ((BLOCKSIZE - 9) < (msgLen % BLOCKSIZE)));

    lenB = (msgTotalLen + msgLen) << 3;
    tempLen = blockNum << 6;

    memset(msgBlock + msgLen, 0, tempLen - msgLen);
    msgBlock[msgLen] = 0x80;

    WORDTOCHAR(lenB, msgBlock + tempLen - 4);
    Transform(msgBlock, blockNum, sha256K, sha256H);

    for (int i = 0; i < 8; i++)
        WORDTOCHAR(sha256H[i], &digest[i << 2]);
}

char* gpu_sha256(const char* input, const WORD* sha256K) {
    // unsigned char* digest = new unsigned char[32]();
    unsigned char* digest = (unsigned char*)malloc(sizeof(unsigned char) * 32);
    unsigned char msgBlock[128];
    WORD msgTotalLen = 0, msgLen = 0;
    WORD sha256H[8]{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

    Update((unsigned char*)input, strlen(input), sha256K, sha256H, msgBlock, msgTotalLen, msgLen);
    Final(digest, sha256K, sha256H, msgBlock, msgTotalLen, msgLen);

    char* buf = (char*)malloc(sizeof(char) * 65);
    buf[64] = 0;
    for (int i = 0; i < 32; i++)
        sprintf(buf + i * 2, "%02x", digest[i]);

    free(digest);
    return buf;
}
#if RUN_ON_TARGET
#pragma omp end declare target
#endif
