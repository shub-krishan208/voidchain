#include "block.h"
#include "./utils/hashing.h"

class Chain {
public:
    Chain();
    // create the block and add to the chain. (mining logic implemented later)
    void addBlock(const std::string data);
    Block getLatestBlock(); //outputs the block at the last of the chain
private:
    std::vector<Block> chain;
};