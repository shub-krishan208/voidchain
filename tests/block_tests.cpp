#include <gtest/gtest.h>
#include "../src/block.h"
#include "../src/models/CurrencyTxn.h"

TEST(BlockTest, GenesisBlock){
    Block genesis = Block::genesis();
    EXPECT_EQ(genesis.getMerkleRoot(), "");
    EXPECT_EQ(genesis.getLastHash(), std::string(64, '0'));
}

TEST(BlockTest, MineBlock){
    Block genesis = Block::genesis();
    auto tx1 = std::make_shared<CurrencyTxn>();
    tx1->id = "tx001";
    tx1->from = "alice";
    tx1->to = "bob";
    tx1->amount = 50.0;
    tx1->signature = "sig_alice_001";

    auto tx2 = std::make_shared<CurrencyTxn>();
    tx2->id = "tx002";
    tx2->from = "bob";
    tx2->to = "charlie";
    tx2->amount = 25.0;
    tx2->signature = "sig_bob_002";

  std::vector<std::shared_ptr<Txn>> txns = {tx1, tx2};


    Block newBlock = Block::mineBlock(genesis, txns);
    // EXPECT_EQ(newBlock.getMerkleRoot(), txns);
    EXPECT_EQ(newBlock.getLastHash(), genesis.getHash());

}