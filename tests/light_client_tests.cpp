#include "../src/TxnPool.h"
#include "../src/chain.h"
#include "../src/core/MerkleTree.h"
#include "../src/miner.h"
#include "../src/models/CurrencyTxn.h"
#include "../src/utils/hashing.h"
#include "../src/wallet.h"
#include <crow/json.h>
#include <gtest/gtest.h>
#include <memory>

namespace {
std::shared_ptr<CurrencyTxn> makeSignedCurrencyTxn(Wallet &wallet,
                                                   const std::string &to,
                                                   double amount) {
  auto txn = std::make_shared<CurrencyTxn>();
  txn->to = to;
  txn->amount = amount;
  wallet.signTxn(*txn);
  return txn;
}
} // namespace

TEST(LightClientTest, HeaderToJsonContainsHeaderFieldsOnly) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet wallet;
  Miner miner(blockchain, pool, wallet);

  Block mined = miner.mine();
  auto header = crow::json::load(mined.headerToJson().dump());
  ASSERT_TRUE(header);
  EXPECT_TRUE(header.has("timestamp"));
  EXPECT_TRUE(header.has("last_hash"));
  EXPECT_TRUE(header.has("hash"));
  EXPECT_TRUE(header.has("merkle_root"));
  EXPECT_TRUE(header.has("nonce"));
  EXPECT_TRUE(header.has("difficulty"));
  EXPECT_FALSE(header.has("transactions"));
}

TEST(LightClientTest, FindTransactionInChainFindsKnownTransaction) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet wallet;
  Miner miner(blockchain, pool, wallet);

  miner.mine(); // funding reward
  auto spend = makeSignedCurrencyTxn(wallet, "bob", 10.0);
  ASSERT_TRUE(pool.addTxn(spend));
  miner.mine();

  size_t blockIndex = 0;
  std::shared_ptr<Txn> foundTxn;
  const auto chainSnapshot = blockchain.getChain();
  ASSERT_TRUE(Blockchain::findTransactionInChain(chainSnapshot, spend->id,
                                                 blockIndex, foundTxn));
  ASSERT_TRUE(foundTxn != nullptr);
  EXPECT_EQ(foundTxn->id, spend->id);
  EXPECT_LT(blockIndex, chainSnapshot.size());
}

TEST(LightClientTest, FindTransactionInChainReturnsFalseForUnknownTransaction) {
  Blockchain blockchain;
  size_t blockIndex = 0;
  std::shared_ptr<Txn> foundTxn;
  const auto chainSnapshot = blockchain.getChain();
  EXPECT_FALSE(Blockchain::findTransactionInChain(chainSnapshot, "missing-tx-id",
                                                  blockIndex, foundTxn));
}

TEST(LightClientTest, ProofDataRoundTripVerifiesAgainstMerkleRoot) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet wallet;
  Miner miner(blockchain, pool, wallet);

  miner.mine(); // funding reward
  auto spend = makeSignedCurrencyTxn(wallet, "charlie", 7.0);
  ASSERT_TRUE(pool.addTxn(spend));
  miner.mine();

  size_t blockIndex = 0;
  std::shared_ptr<Txn> foundTxn;
  const auto chainSnapshot = blockchain.getChain();
  ASSERT_TRUE(Blockchain::findTransactionInChain(chainSnapshot, spend->id,
                                                 blockIndex, foundTxn));
  ASSERT_TRUE(foundTxn != nullptr);

  const Block &block = chainSnapshot[blockIndex];
  MerkleTree tree(block.getTransactions());

  Hasher txHasher;
  txHasher.add(foundTxn->toJson().dump());
  const std::string txHash = txHasher.finish();

  const auto proof = tree.getProof(txHash);
  EXPECT_TRUE(MerkleTree::verifyProof(block.getMerkleRoot(), txHash, proof));
  EXPECT_EQ(tree.getRoot(), block.getMerkleRoot());
}
