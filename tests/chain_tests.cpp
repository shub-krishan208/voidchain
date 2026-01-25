#include <gtest/gtest.h>
#include "../src/chain.h"

TEST(ChainTest, InitializeChain) {
    Chain chain;
    Block latestBlock = chain.getLatestBlock();
    Block genesisBlock = Block::genesis();

    // EXPECT_EQ requires objects to be comparable!
    // EXPECT_EQ(latestBlock, genesisBlock);
    EXPECT_EQ(latestBlock.getData(), genesisBlock.getData());
    EXPECT_EQ(latestBlock.getLastHash(), genesisBlock.getLastHash());
    
}

TEST(ChainTest, AddBlock) {
    Chain chain;
    std::string data = "Test Block Data";
    chain.addBlock(data);

    Block latestBlock = chain.getLatestBlock();
    EXPECT_EQ(latestBlock.getData(), data);
}