#include "../src/block.h"
#include "../src/core/TxnFactory.h"
#include "../src/models/CurrencyTxn.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <string>

namespace {
bool hasLeadingZeros(const std::string &hash, int difficulty) {
  return hash.substr(0, static_cast<size_t>(difficulty)) ==
         std::string(static_cast<size_t>(difficulty), '0');
}
} // namespace

class BlockTest : public ::testing::Test {
protected:
  void SetUp() override {
    TxnFactory::registerType("CURRENCY", CurrencyTxn::fromJson);
  }
};

TEST_F(BlockTest, GenesisBlock) {
  Block genesis = Block::genesis();
  EXPECT_EQ(genesis.getMerkleRoot(), "");
  EXPECT_EQ(genesis.getLastHash(), std::string(64, '0'));
  EXPECT_EQ(genesis.getNonce(), 0);
  EXPECT_EQ(genesis.getDifficulty(), Block::INITIAL_DIFFICULTY);
}

TEST_F(BlockTest, MineBlock) {
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
  EXPECT_EQ(newBlock.getLastHash(), genesis.getHash());
  EXPECT_GT(newBlock.getNonce(), 0);
  EXPECT_GE(newBlock.getDifficulty(), 1);
  EXPECT_TRUE(hasLeadingZeros(newBlock.getHash(), newBlock.getDifficulty()));
}

TEST_F(BlockTest, HashBlockChangesWithNonceAndDifficulty) {
  std::string timestamp = "123456";
  std::string previousHash(64, 'a');
  std::string merkleRoot(64, 'b');

  std::string hash1 =
      Block::hashBlock(timestamp, previousHash, merkleRoot, 1, 2);
  std::string hash2 =
      Block::hashBlock(timestamp, previousHash, merkleRoot, 2, 2);
  std::string hash3 =
      Block::hashBlock(timestamp, previousHash, merkleRoot, 1, 3);

  EXPECT_NE(hash1, hash2);
  EXPECT_NE(hash1, hash3);
}

TEST_F(BlockTest, AdjustDifficultyIncreasesWhenBlockMinedTooFast) {
  Block lastBlock = Block::genesis();
  int adjusted = Block::adjustDifficulty(
      lastBlock, lastBlock.getTimestamp() + Block::MINE_RATE_MS - 1);
  EXPECT_EQ(adjusted, lastBlock.getDifficulty() + 1);
}

TEST_F(BlockTest, AdjustDifficultyDecreasesWhenBlockMinedTooSlow) {
  Block lastBlock = Block::genesis();
  int adjusted = Block::adjustDifficulty(
      lastBlock, lastBlock.getTimestamp() + Block::MINE_RATE_MS + 1);
  EXPECT_EQ(adjusted, std::max(1, lastBlock.getDifficulty() - 1));
}

TEST_F(BlockTest, FromJsonRoundTrip) {
  Block genesis = Block::genesis();

  auto tx1 = std::make_shared<CurrencyTxn>();
  tx1->id = "tx001";
  tx1->from = "alice";
  tx1->to = "bob";
  tx1->amount = 50.0;
  tx1->signature = "sig_alice_001";

  std::vector<std::shared_ptr<Txn>> txns = {tx1};

  Block mined = Block::mineBlock(genesis, txns);

  // serialize to crow JSON
  crow::json::wvalue wval = mined.toJson();
  std::string jsonStr = wval.dump();

  // parse back into rvalue
  crow::json::rvalue rval = crow::json::load(jsonStr);

  Block restored = Block::fromJson(rval);

  EXPECT_EQ(restored.getTimestamp(), mined.getTimestamp());
  EXPECT_EQ(restored.getLastHash(), mined.getLastHash());
  EXPECT_EQ(restored.getHash(), mined.getHash());
  EXPECT_EQ(restored.getMerkleRoot(), mined.getMerkleRoot());
  EXPECT_EQ(restored.getNonce(), mined.getNonce());
  EXPECT_EQ(restored.getDifficulty(), mined.getDifficulty());
}

TEST_F(BlockTest, FromJsonMultipleTransactions) {
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

  auto tx3 = std::make_shared<CurrencyTxn>();
  tx3->id = "tx003";
  tx3->from = "charlie";
  tx3->to = "dave";
  tx3->amount = 10.0;
  tx3->signature = "sig_charlie_003";

  std::vector<std::shared_ptr<Txn>> txns = {tx1, tx2, tx3};

  Block mined = Block::mineBlock(genesis, txns);

  crow::json::wvalue wval = mined.toJson();
  std::string jsonStr = wval.dump();
  crow::json::rvalue rval = crow::json::load(jsonStr);

  Block restored = Block::fromJson(rval);

  EXPECT_EQ(restored.getTimestamp(), mined.getTimestamp());
  EXPECT_EQ(restored.getLastHash(), mined.getLastHash());
  EXPECT_EQ(restored.getHash(), mined.getHash());
  EXPECT_EQ(restored.getMerkleRoot(), mined.getMerkleRoot());
  EXPECT_EQ(restored.getNonce(), mined.getNonce());
  EXPECT_EQ(restored.getDifficulty(), mined.getDifficulty());
}

TEST_F(BlockTest, FromJsonEmptyTransactions) {
  Block genesis = Block::genesis();

  std::vector<std::shared_ptr<Txn>> emptyTxns;
  Block mined = Block::mineBlock(genesis, emptyTxns);

  crow::json::wvalue wval = mined.toJson();
  std::string jsonStr = wval.dump();
  crow::json::rvalue rval = crow::json::load(jsonStr);

  Block restored = Block::fromJson(rval);

  EXPECT_EQ(restored.getTimestamp(), mined.getTimestamp());
  EXPECT_EQ(restored.getLastHash(), mined.getLastHash());
  EXPECT_EQ(restored.getHash(), mined.getHash());
  EXPECT_EQ(restored.getMerkleRoot(), mined.getMerkleRoot());
  EXPECT_EQ(restored.getNonce(), mined.getNonce());
  EXPECT_EQ(restored.getDifficulty(), mined.getDifficulty());
}

TEST_F(BlockTest, FromJsonMerkleRootMismatch) {
  Block genesis = Block::genesis();

  auto tx1 = std::make_shared<CurrencyTxn>();
  tx1->id = "tx001";
  tx1->from = "alice";
  tx1->to = "bob";
  tx1->amount = 50.0;
  tx1->signature = "sig_alice_001";

  std::vector<std::shared_ptr<Txn>> txns = {tx1};

  Block mined = Block::mineBlock(genesis, txns);

  crow::json::wvalue wval = mined.toJson();
  // tamper with the merkle root to force a mismatch
  wval["merkle_root"] = "invalid_merkle_root_value";

  std::string jsonStr = wval.dump();
  crow::json::rvalue rval = crow::json::load(jsonStr);

  EXPECT_THROW(Block::fromJson(rval), std::runtime_error);
}