#include "chain.h"

Chain::Chain() {
    // initialize with the genesis block
    chain.push_back(Block::genesis());
}

void Chain::addBlock(const std::string data) {
    Block newTxn = Block::mineBlock(getLatestBlock(), data);
    chain.push_back(newTxn);
}

Block Chain::getLatestBlock() {
    return chain.back();
}