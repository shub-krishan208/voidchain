#include "block.h"
#include "./utils/hashing.h"

class Blockchain {
public:
    Blockchain();
    // create the block and add to the chain. (mining logic implemented later)
    const std::vector<Block>& getChain() const {return chain; };
    void addBlock(const std::string data);
    const Block& getLatestBlock(); //outputs the block at the last of the chain
    static bool isValidBlockchain(const std::vector<Block>& newchain);
    void replaceBlockchain(const std::vector<Block>& newBlockchain);
private:
    std::vector<Block> chain;
};