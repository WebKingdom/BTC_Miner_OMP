#include <string>

using namespace std;

class Blockchain {
   private:
    // Block class. Used to store the data for each block in the blockchain.
    class Block {
       public:
        size_t block_id;
        string prev_digest;
        string data;
        size_t threshold;
        size_t nonce;
        Block *next;
    };
    Block *head;
    Block *current;
    size_t num_blocks;
    size_t block_counter;

   public:
    Blockchain();
    ~Blockchain();
    bool isEmpty() { return (head == NULL); }
    void appendBlock(string prev_digest, string data, size_t threshold, size_t nonce);
    void removeBlock();
    bool thresholdMet(string &digest, size_t &threshold);
    string getString(size_t &cur_nonce);
    size_t getCurrentBlockId() { return current->block_id; }
    string getPrevDigest() { return current->prev_digest; }
    size_t getSize() { return num_blocks; }
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
void Blockchain::appendBlock(string prev_digest, string data, size_t threshold, size_t nonce) {
    Block *new_block = new Block();
    new_block->block_id = block_counter;
    new_block->nonce = nonce;
    new_block->prev_digest = prev_digest;
    new_block->data = data;
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
 * @brief Removes a block from the front of the blockchain.
 *
 */
void Blockchain::removeBlock() {
    if (!isEmpty()) {
        Block *temp = head;
        head = head->next;
        delete temp;
        num_blocks--;
    }
}

/**
 * @brief Checks if the digest has exactly threshold number of leading zeros.
 *
 * @param digest
 * @param threshold
 * @return true
 * @return false
 */
bool Blockchain::thresholdMet(string &digest, size_t &threshold) {
    if (threshold >= digest.length()) {
        // Cannot have more leading zeros than the length of the digest.
        return false;
    }

    for (size_t i = 0; i < threshold; i++) {
        if (digest[i] != '0') {
            return false;
        }
    }

    // If the current digest has more than the threshold number of leading zeros, then it is not a valid digest.
    if (digest[threshold] == '0') {
        return false;
    }

    return true;
}

/**
 * @brief Returns the string representation of the current block.
 *
 * @return string
 */
string Blockchain::getString(size_t &cur_nonce) {
    return "[" + to_string(current->block_id) + "|" + current->prev_digest + "|" + current->data + "|" + to_string(current->threshold) + "|" + to_string(cur_nonce) + "]";
}
