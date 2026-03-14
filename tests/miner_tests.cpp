#include "../src/TxnPool.h"
#include "../src/chain.h"
#include "../src/miner.h"
#include "../src/models/AssetTxn.h"
#include "../src/models/CurrencyTxn.h"
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

std::shared_ptr<AssetTxn> makeSignedAssetTxn(Wallet &wallet,
                                             const std::string &to,
                                             const std::string &itemId,
                                             const std::string &meta) {
  auto txn = std::make_shared<AssetTxn>();
  txn->to = to;
  txn->itemId = itemId;
  txn->meta = meta;
  wallet.signTxn(*txn);
  return txn;
}
} // namespace

TEST(MinerTest, MineThrowsWhenPoolIsEmpty) {
  Blockchain blockchain;
  TxnPool pool;
  Miner miner(blockchain, pool);

  EXPECT_THROW(miner.mine(), std::runtime_error);
  EXPECT_EQ(blockchain.getChain().size(), 1u);
}

TEST(MinerTest, MineIncludesPendingTransactionsAndClearsMinedOnes) {
  Blockchain blockchain;
  TxnPool pool;
  Miner miner(blockchain, pool);
  Wallet wallet;

  auto tx1 = makeSignedCurrencyTxn(wallet, "bob", 10.0);
  auto tx2 = makeSignedAssetTxn(wallet, "charlie", "item-77", "epic");

  ASSERT_TRUE(pool.addTxn(tx1));
  ASSERT_TRUE(pool.addTxn(tx2));
  ASSERT_EQ(pool.getTxn().size(), 2u);

  std::string previousHash = blockchain.getLatestBlock().getHash();
  Block mined = miner.mine();

  EXPECT_EQ(blockchain.getChain().size(), 2u);
  EXPECT_EQ(mined.getLastHash(), previousHash);
  EXPECT_TRUE(pool.getTxn().empty());

  auto minedJson = crow::json::load(mined.toJson().dump());
  ASSERT_TRUE(minedJson);
  ASSERT_EQ(minedJson["transactions"].size(), 2u);
}

