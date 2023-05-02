/**
 * Blockchain class. Used to store the data for the entire blockchain like a linked list.
*/
class Blockchain {
   public:
    // Block class. Used to store the data for each block in the blockchain.
    class Block {
       public:
        size_t block_id;
        char *prev_digest;
        char *data;
        size_t threshold;
        size_t nonce;
        Block *next;
    };
    Block *head;
    Block *current;
    size_t num_blocks;
    size_t block_counter;

    Blockchain();
    ~Blockchain();
    void removeBlock();
    void print();
    int isEmpty() { return (head == NULL); }
    size_t getCurrentBlockId() { return current->block_id; }
    Block *getCurrentBlock() { return current; }
    char *getPrevDigest() { return current->prev_digest; }
    size_t getSize() { return num_blocks; }
    void appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce);
    int thresholdMet(const char *digest, size_t &threshold);
    char *getString(size_t &cur_nonce);
    char *size_t_to_string(size_t num);

#if RUN_ON_TARGET
#pragma omp declare target
#endif
    int t_isEmpty() { return (head == NULL); }
    size_t t_getCurrentBlockId() { return current->block_id; }
    Block *t_getCurrentBlock() { return current; }
    char *t_getPrevDigest() { return current->prev_digest; }
    size_t t_getSize() { return num_blocks; }
    void t_appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce);
    int t_thresholdMet(const char *digest, size_t &threshold);
    // char *t_getString(size_t &cur_nonce, Block *current);
    char *t_makeString(size_t &cur_nonce, size_t block_id, const char *prev_digest, const char *data, size_t threshold);
    char *t_size_t_to_string(size_t num);
#if RUN_ON_TARGET
#pragma omp end declare target
#endif
};

/**
 * @brief Construct a new Blockchain object
 *
 */
Blockchain::Blockchain() {
    head = NULL;
    current = NULL;
    num_blocks = 0;
    block_counter = 0;
}

/**
 * @brief Destroy the Blockchain object
 *
 */
Blockchain::~Blockchain() {
    while (!isEmpty()) {
        removeBlock();
    }
}

/**
 * @brief Appends a block to the end of the blockchain. Beyond the current pointer.
 *
 * @param prev_digest
 * @param data
 * @param threshold
 * @param nonce
 */
void Blockchain::appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce) {
    Block *new_block = (Block *)malloc(sizeof(Block));
    new_block->block_id = block_counter;
    new_block->nonce = nonce;
    // deep copy the strings
    new_block->prev_digest = (char *)malloc(sizeof(char) * (strlen(prev_digest) + 1));
    new_block->data = (char *)malloc(sizeof(char) * (strlen(data) + 1));
    strcpy(new_block->prev_digest, prev_digest);
    strcpy(new_block->data, data);
    new_block->threshold = threshold;
    new_block->next = NULL;

    if (isEmpty()) {
        head = new_block;
        current = new_block;
    } else {
        current->next = new_block;
        current = new_block;
    }
    num_blocks++;
    block_counter++;
}

/**
 * @brief Appends a block to the end of the blockchain. Beyond the current pointer.
 *
 * @param prev_digest
 * @param data
 * @param threshold
 * @param nonce
 */
void Blockchain::t_appendBlock(const char *prev_digest, const char *data, size_t threshold, size_t nonce) {
    Block *new_block = (Block *)malloc(sizeof(Block));
    new_block->block_id = block_counter;
    new_block->nonce = nonce;
    // deep copy the strings
    new_block->prev_digest = (char *)malloc(sizeof(char) * (strlen(prev_digest) + 1));
    new_block->data = (char *)malloc(sizeof(char) * (strlen(data) + 1));
    strcpy(new_block->prev_digest, prev_digest);
    strcpy(new_block->data, data);
    new_block->threshold = threshold;
    new_block->next = NULL;

    if (t_isEmpty()) {
        head = new_block;
        current = new_block;
    } else {
        current->next = new_block;
        current = new_block;
    }
    num_blocks++;
    block_counter++;
}

/**
 * @brief Removes a block from the front of the blockchain.
 *
 */
void Blockchain::removeBlock() {
    if (!isEmpty()) {
        Block *temp = head;
        head = head->next;
        free(temp->prev_digest);
        free(temp->data);
        free(temp);
        num_blocks--;
    }
}

/**
 * @brief Checks if the digest has exactly threshold number of leading zeros.
 *
 * @param digest
 * @param threshold
 * @return true - 1
 * @return false - 0
 */
int Blockchain::thresholdMet(const char *digest, size_t &threshold) {
    unsigned char valid = 1;
    if (threshold >= strlen(digest)) {
        // Cannot have more leading zeros than the length of the digest.
        valid = 0;
    }

    for (size_t i = 0; i < threshold; i++) {
        if (digest[i] != '0') {
            valid = 0;
        }
    }

    // If the current digest has more than the threshold number of leading zeros, then it is not a valid digest.
    if (digest[threshold] == '0') {
        valid = 0;
    }

    return valid;
}

/**
 * @brief Checks if the digest has exactly threshold number of leading zeros.
 *
 * @param digest
 * @param threshold
 * @return true - 1
 * @return false - 0
 */
int Blockchain::t_thresholdMet(const char *digest, size_t &threshold) {
    unsigned char valid = 1;
    if (threshold >= strlen(digest)) {
        // Cannot have more leading zeros than the length of the digest.
        valid = 0;
    }

    for (size_t i = 0; i < threshold; i++) {
        if (digest[i] != '0') {
            valid = 0;
        }
    }

    // If the current digest has more than the threshold number of leading zeros, then it is not a valid digest.
    if (digest[threshold] == '0') {
        valid = 0;
    }

    return valid;
}

/**
 * @brief Returns the string representation of the current block.
 *
 * @return char*
 */
char *Blockchain::getString(size_t &cur_nonce) {
    // * original
    char *str = (char *)malloc(sizeof(char) * (1 + SIZE_T_STR_BYTES + 1 + strlen(current->prev_digest) + 1 + strlen(current->data) + 1 + SIZE_T_STR_BYTES + 1 + SIZE_T_STR_BYTES + 2));
    sprintf(str, "[%lu|%s|%s|%lu|%lu]", current->block_id, current->prev_digest, current->data, current->threshold, cur_nonce);

    // TODO without sprintf
    // char *str_nonce = size_t_to_string(cur_nonce);
    // char *str_block_id = size_t_to_string(current->block_id);
    // char *str_threshold = size_t_to_string(current->threshold);
    // char *str = (char *)malloc(sizeof(char) * (1 + strlen(str_block_id) + 2 + strlen(current->prev_digest) + 2 + strlen(current->data) + 2 + strlen(str_threshold) + 2 + strlen(str_nonce) + 3));
    // sprintf(str, "[%s|%s|%s|%s|%s]", str_block_id, current->prev_digest, current->data, str_threshold, str_nonce);
    // strcpy(str, "[");
    // strcat(str, str_block_id);
    // strcat(str, "|");
    // strcat(str, current->prev_digest);
    // strcat(str, "|");
    // strcat(str, current->data);
    // strcat(str, "|");
    // strcat(str, str_threshold);
    // strcat(str, "|");
    // strcat(str, str_nonce);
    // strcat(str, "]");
    // free(str_nonce);
    // free(str_block_id);
    // free(str_threshold);
    return str;
}

char *Blockchain::size_t_to_string(size_t num) {
    unsigned int num_digits = 1;
    size_t temp = num;
    while (temp /= 10) {
        num_digits++;
    }

    char *str = (char *)malloc(sizeof(char) * (num_digits + 1));
    if (num == 0) {
        strcpy(str, "0");
    } else {
        unsigned int i = 0;
        temp = num;
        while (temp > 0) {
            str[i] = (temp % 10) + '0';
            temp /= 10;
            i++;
        }
    }
    return str;
}

/**
 * @brief Returns the string representation of the current block.
 *
 * @return char*
 */
// char *Blockchain::t_getString(size_t &cur_nonce, Block *current) {
//     char *str = (char *)malloc(sizeof(char) * (1 + SIZE_T_STR_BYTES + 1 + strlen(current->prev_digest) + 1 + strlen(current->data) + 1 + SIZE_T_STR_BYTES + 1 + SIZE_T_STR_BYTES + 2));
//     sprintf(str, "[%lu|%s|%s|%lu|%lu]", current->block_id, current->prev_digest, current->data, current->threshold, cur_nonce);
//     return str;
// }

/**
 * @brief Returns the string representation of the current block.
 *
 * @return char*
 */
char *Blockchain::t_makeString(size_t &cur_nonce, size_t block_id, const char *prev_digest, const char *data, size_t threshold) {
    // * original
    char *str = (char *)malloc(sizeof(char) * (1 + SIZE_T_STR_BYTES + 1 + strlen(prev_digest) + 1 + strlen(data) + 1 + SIZE_T_STR_BYTES + 1 + SIZE_T_STR_BYTES + 2));
    sprintf(str, "[%lu|%s|%s|%lu|%lu]", block_id, prev_digest, data, threshold, cur_nonce);

    // TODO without sprintf
    // char *str_nonce = t_size_t_to_string(cur_nonce);
    // char *str_block_id = t_size_t_to_string(block_id);
    // char* str_threshold = t_size_t_to_string(threshold);
    // char *str = (char *)malloc(sizeof(char) * (1 + strlen(str_block_id) + 2 + strlen(prev_digest) + 2 + strlen(data) + 2 + strlen(str_threshold) + 2 + strlen(str_nonce) + 3));
    // sprintf(str, "[%s|%s|%s|%s|%s]", str_block_id, prev_digest, data, str_threshold, str_nonce);
    // strcpy(str, "[");
    // strcat(str, str_block_id);
    // strcat(str, "|");
    // strcat(str, prev_digest);
    // strcat(str, "|");
    // strcat(str, data);
    // strcat(str, "|");
    // strcat(str, str_threshold);
    // strcat(str, "|");
    // strcat(str, str_nonce);
    // strcat(str, "]");
    // free(str_nonce);
    // free(str_block_id);
    // free(str_threshold);
    return str;
}

char *Blockchain::t_size_t_to_string(const size_t num) {
    unsigned int num_digits = 1;
    size_t temp = num;
    while (temp /= 10) {
        num_digits++;
    }

    char *str = (char *)malloc(sizeof(char) * (num_digits + 1));
    if (num == 0) {
        strcpy(str, "0");
    } else {
        unsigned int i = 0;
        temp = num;
        while (temp > 0) {
            str[i] = (temp % 10) + '0';
            temp /= 10;
            i++;
        }
    }
    return str;
}

/**
 * @brief Prints the blockchain.
 *
 */
void Blockchain::print() {
    Block *temp = head;
    printf("\nBlockchain with %lu blocks:\n", num_blocks);
    while (temp != NULL) {
        printf("[%lu|%s|%s|%lu|%lu]\n\n", temp->block_id, temp->prev_digest, temp->data, temp->threshold, temp->nonce);
        temp = temp->next;
    }
}
