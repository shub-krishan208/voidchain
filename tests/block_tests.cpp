#include <gtest/gtest.h>
#include "../src/block.h"

TEST(BlockTest, GenesisBlock){
    Block genesis = Block::genesis();
    EXPECT_EQ(genesis.getData(), "");
    EXPECT_EQ(genesis.getLastHash(), std::string(64, '0'));
}

TEST(BlockTest, MineBlock){
    Block genesis = Block::genesis();
    std::string data = "first Transaction Block";

    Block newBlock = Block::mineBlock(genesis, data);
    EXPECT_EQ(newBlock.getData(), data);
    EXPECT_EQ(newBlock.getLastHash(), genesis.getHash());

}