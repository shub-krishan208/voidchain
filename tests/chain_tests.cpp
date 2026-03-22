#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "../src/chain.h"
#include "../src/core/TxnFactory.h"
#include "../src/utils/TimeUtils.h"
#include "../src/models/CurrencyTxn.h"

using str=std::string;
#define RESET "\x1B[0m"
#define ERROR "\x1B[31m"
#define SUCCESS "\x1B[32m"
#define WARNING "\x1B[33m"

// void printBlock(str title, Block b) {
//   std::cout << title << "\n"
//             << "Timestamp: " << getFormattedTimestamp(b.getTimestamp()) << "\n"
//             << "Last Hash: " << b.getLastHash() << "\n"
//             << "Hash: " << b.getHash() << "\n"
//             << "Data: " << b.getMerkleRoot() << std::endl;
// }



TEST(BlockchainTest, InitializeBlockchain) {
    Blockchain chain;
    Block latestBlock = chain.getLatestBlock();
    Block genesisBlock = Block::genesis();

    // EXPECT_EQ requires objects to be comparable!
    // EXPECT_EQ(latestBlock, genesisBlock);
    EXPECT_EQ(latestBlock.getMerkleRoot(), genesisBlock.getMerkleRoot());
    EXPECT_EQ(latestBlock.getLastHash(), genesisBlock.getLastHash());
    
}

TEST(BlockchainTest, AddBlock) {
    Blockchain chain;

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


    chain.addBlock(txns);

    Block latestBlock = chain.getLatestBlock();
    EXPECT_NE(latestBlock.getMerkleRoot(), "");
}

TEST(BlockchainTest, ValidateGenesisBlock) {
    // single block chain validates genesis block
    EXPECT_TRUE(Blockchain::isValidBlockchain({Block::genesis()}));
}


TEST(BlockchainTest, ValidateBlockchain) {
    Blockchain chain;

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


    chain.addBlock(txns);
    
    std::vector<Block> validchain = {Block::genesis(), chain.getLatestBlock()};
    EXPECT_TRUE(Blockchain::isValidBlockchain(validchain));
}

TEST(BlockchainTest, InValidateGenesisBlock) {
    // single block chain validates genesis block
    std::vector<std::shared_ptr<Txn>> emptyTxs;
    EXPECT_FALSE(Blockchain::isValidBlockchain({Block::mineBlock(Block::genesis(), emptyTxs)}));
}

TEST(BlockchainTest, InvalidateBlockchain) {
    Blockchain chain;

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

  std::vector<std::shared_ptr<Txn>> txns = {tx1};


    chain.addBlock(txns);
    txns.push_back(tx2);
    chain.addBlock(txns);

    std::vector<Block> validchain = {Block::genesis(), chain.getLatestBlock()};
    EXPECT_FALSE(Blockchain::isValidBlockchain(validchain));
}

TEST(BlockchainTest, ReplaceValidBlockchain) {
    Blockchain chain;
    std::vector<std::shared_ptr<Txn>> emptyTxs;
    chain.addBlock(emptyTxs);
    chain.addBlock(emptyTxs);

    // newBlockchain must be longer than chain for replaceBlockchain to accept it
    Blockchain newBlockchain;
    newBlockchain.addBlock(emptyTxs);
    newBlockchain.addBlock(emptyTxs);
    newBlockchain.addBlock(emptyTxs); // 4 blocks total including genesis

    EXPECT_NO_THROW(chain.replaceBlockchain(newBlockchain.getChain()));
    EXPECT_EQ(chain.getChain().size(), newBlockchain.getChain().size());
}

TEST(BlockchainTest, RejectInvalidChainReplacement) {
    Blockchain blockchain;
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
    // need at bc of at least 3 blocks to invalidate as there is no consensus implemented yet
    blockchain.addBlock(txns);
    txns.push_back(tx2);
    blockchain.addBlock(txns); 
    
    // copy the blockchain AFTER adding blocks, then corrupt it
    std::vector<Block> corruptedChain = blockchain.getChain();

    std::vector<std::shared_ptr<Txn>> emptyTxs;
    corruptedChain[1] = Block::mineBlock(Block::genesis(), emptyTxs);
    // try replacing with the same length (should be rejected as not longer)
    EXPECT_THROW({
        blockchain.replaceBlockchain(corruptedChain);
    }, std::invalid_argument);
    
    corruptedChain.push_back(Block::mineBlock(corruptedChain.back(), txns));

    // std::cout<<ERROR<<std::endl;
    // for(const auto& block : corruptedChain){
    //     printBlock("Block in Corrupted Chain", block);
    // }
    // std::cout<<RESET<<std::endl;

    EXPECT_THROW({
        blockchain.replaceBlockchain(corruptedChain);
    }, std::invalid_argument);
}

TEST(BlockchainTest, RejectBlockWithInvalidDifficultyRequirement) {
  TxnFactory::registerType("CURRENCY", CurrencyTxn::fromJson);

  auto tx1 = std::make_shared<CurrencyTxn>();
  tx1->id = "tx001";
  tx1->from = "alice";
  tx1->to = "bob";
  tx1->amount = 50.0;
  tx1->signature = "sig_alice_001";
  std::vector<std::shared_ptr<Txn>> txns = {tx1};

  Block genesis = Block::genesis();
  Block mined = Block::mineBlock(genesis, txns);

  auto json = mined.toJson();
  json["difficulty"] = 64;
  auto tampered = Block::fromJson(crow::json::load(json.dump()));

  EXPECT_FALSE(Blockchain::isValidBlock(tampered, genesis));
}

TEST(BlockchainTest, RejectBlockWithTamperedNonceOrHashInputs) {
  TxnFactory::registerType("CURRENCY", CurrencyTxn::fromJson);

  auto tx1 = std::make_shared<CurrencyTxn>();
  tx1->id = "tx001";
  tx1->from = "alice";
  tx1->to = "bob";
  tx1->amount = 50.0;
  tx1->signature = "sig_alice_001";
  std::vector<std::shared_ptr<Txn>> txns = {tx1};

  Block genesis = Block::genesis();
  Block mined = Block::mineBlock(genesis, txns);

  auto json = mined.toJson();
  json["nonce"] = mined.getNonce() + 1;
  auto tampered = Block::fromJson(crow::json::load(json.dump()));

  EXPECT_FALSE(Blockchain::isValidBlock(tampered, genesis));
}