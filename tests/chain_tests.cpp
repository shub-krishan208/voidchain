#include <gtest/gtest.h>
#include "../src/chain.h"
#include "../src/utils/TimeUtils.h"

using str=std::string;
#define RESET "\x1B[0m"
#define ERROR "\x1B[31m"
#define SUCCESS "\x1B[32m"
#define WARNING "\x1B[33m"

void printBlock(str title, Block b) {
  std::cout << title << "\n"
            << "Timestamp: " << getFormattedTimestamp(b.getTimestamp()) << "\n"
            << "Last Hash: " << b.getLastHash() << "\n"
            << "Hash: " << b.getHash() << "\n"
            << "Data: " << b.getData() << std::endl;
}

TEST(BlockchainTest, InitializeBlockchain) {
    Blockchain chain;
    Block latestBlock = chain.getLatestBlock();
    Block genesisBlock = Block::genesis();

    // EXPECT_EQ requires objects to be comparable!
    // EXPECT_EQ(latestBlock, genesisBlock);
    EXPECT_EQ(latestBlock.getData(), genesisBlock.getData());
    EXPECT_EQ(latestBlock.getLastHash(), genesisBlock.getLastHash());
    
}

TEST(BlockchainTest, AddBlock) {
    Blockchain chain;
    std::string data = "Test Block Data";
    chain.addBlock(data);

    Block latestBlock = chain.getLatestBlock();
    EXPECT_EQ(latestBlock.getData(), data);
}

TEST(BlockchainTest, ValidateGenesisBlock) {
    // single block chain validates genesis block
    EXPECT_TRUE(Blockchain::isValidBlockchain({Block::genesis()}));
}


TEST(BlockchainTest, ValidateBlockchain) {
    Blockchain chain;
    chain.addBlock("Block 1 Data");
    
    std::vector<Block> validchain = {Block::genesis(), chain.getLatestBlock()};
    EXPECT_TRUE(Blockchain::isValidBlockchain(validchain));
}

TEST(BlockchainTest, InValidateGenesisBlock) {
    // single block chain validates genesis block
    EXPECT_FALSE(Blockchain::isValidBlockchain({Block::mineBlock(Block::genesis(), "Corrupted Genesis Block")}));
}

TEST(BlockchainTest, InvalidateBlockchain) {
    Blockchain chain;
    chain.addBlock("Block 1 Data");
    chain.addBlock("Block 2");

    std::vector<Block> validchain = {Block::genesis(), chain.getLatestBlock()};
    EXPECT_FALSE(Blockchain::isValidBlockchain(validchain));
}

TEST(BlockchainTest, ReplaceValidBlockchain) {
    Blockchain chain;
    chain.addBlock("Block 1");
    
    Blockchain newBlockchain;
    newBlockchain.addBlock("Block 1");
    newBlockchain.addBlock("Block 2");

    chain.replaceBlockchain(newBlockchain.getChain());
}

TEST(BlockchainTest, RejectInvalidChainReplacement) {
    Blockchain blockchain;
    // need at bc of at least 3 blocks to invalidate as there is no consensus implemented yet
    blockchain.addBlock("Block 1");
    blockchain.addBlock("Block 2"); 
    
    // std::cout<<WARNING<<std::endl;
    // for(const auto& block : blockchain.getChain()){
    //     printBlock("Block in Corrupted Chain", block);
    // }
    // std::cout<<RESET<<std::endl;

    // copy the blockchain and try corrupting the last block
    std::vector<Block> corruptedChain = blockchain.getChain();
    corruptedChain[1] = Block::mineBlock(Block::genesis(), "Corrupted Data");
    // try replacing with the same lenght
    EXPECT_THROW({
        blockchain.replaceBlockchain(corruptedChain);
    }, std::invalid_argument);
    
    corruptedChain.push_back(Block::mineBlock(corruptedChain.back(), "New Block"));

    // std::cout<<ERROR<<std::endl;
    // for(const auto& block : corruptedChain){
    //     printBlock("Block in Corrupted Chain", block);
    // }
    // std::cout<<RESET<<std::endl;

    EXPECT_THROW({
        blockchain.replaceBlockchain(corruptedChain);
    }, std::invalid_argument);
}