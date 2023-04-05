#include <stdio.h>
#include <stdlib.h>

#include "Blockchain.cpp"

using namespace std;

int main() {
    const string init_prev_digest = "0000000000000000000000000000000000000000000000000000000000000000";
    const string init_data = "This is the initial data in the 1st block";

    size_t global_threshold = 1;
    size_t global_nonce = 0;

    Blockchain blockchain;
    blockchain.appendBlock(init_prev_digest, init_data, global_threshold, global_nonce);

    cout << "Appended Block with ID: " << blockchain.getCurrentBlockId() << endl;
    cout << "Blockchain size: " << blockchain.getSize() << endl;

    string chain_str = blockchain.getString();
    cout << "Blockchain string: " << chain_str << endl;

    return 0;
}
