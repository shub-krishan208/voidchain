#include "../src/TxnPool.h"
#include "../src/chain.h"
#include "../src/core/TxnFactory.h"
#include "../src/models/AssetTxn.h"
#include "../src/models/CurrencyTxn.h"
#include "../src/miner.h"
#include "../src/p2p_server.h"
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

class P2pServerTest : public ::testing::Test {
protected:
  void SetUp() override {
    TxnFactory::registerType("CURRENCY", CurrencyTxn::fromJson);
    TxnFactory::registerType("ASSET", AssetTxn::fromJson);
  }
};

TEST_F(P2pServerTest, AcceptsValidIncomingTransactionMessage) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet wallet;
  P2pServer p2p;
  Miner miner(blockchain, pool, wallet);
  miner.mine();

  auto txn = makeSignedCurrencyTxn(wallet, "bob", 14.0);
  std::string msg = P2pServer::makeTransactionMessage(*txn);

  EXPECT_TRUE(p2p.onPeerMessage(msg, blockchain, pool));
  ASSERT_EQ(pool.getTxn().size(), 1u);
  EXPECT_EQ(pool.getTxn()[0]->id, txn->id);
}

TEST_F(P2pServerTest, RejectsInvalidIncomingTransactionMessage) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet wallet;
  P2pServer p2p;

  auto txn = makeSignedCurrencyTxn(wallet, "bob", 14.0);
  txn->amount = 9999.0; // tamper after signing
  std::string msg = P2pServer::makeTransactionMessage(*txn);

  EXPECT_FALSE(p2p.onPeerMessage(msg, blockchain, pool));
  EXPECT_TRUE(pool.getTxn().empty());
}

TEST_F(P2pServerTest, AcceptsLongerValidIncomingChainMessage) {
  Blockchain localChain;
  Blockchain remoteChain;
  TxnPool pool;
  P2pServer p2p;

  std::vector<std::shared_ptr<Txn>> emptyTxs;
  remoteChain.addBlock(emptyTxs);
  remoteChain.addBlock(emptyTxs);

  std::string msg = P2pServer::makeChainMessage(remoteChain);
  EXPECT_TRUE(p2p.onPeerMessage(msg, localChain, pool));
  EXPECT_EQ(localChain.getChain().size(), remoteChain.getChain().size());
}

TEST_F(P2pServerTest, RejectsInvalidIncomingChainMessage) {
  Blockchain localChain;
  Blockchain remoteChain;
  TxnPool pool;
  Wallet wallet;
  P2pServer p2p;

  auto tx1 = makeSignedCurrencyTxn(wallet, "bob", 3.0);
  remoteChain.addBlock({tx1});

  auto msg = crow::json::load(P2pServer::makeChainMessage(remoteChain));
  ASSERT_TRUE(msg);
  crow::json::wvalue tampered = msg;
  tampered["data"][1]["hash"] = "invalid-hash";

  EXPECT_FALSE(p2p.onPeerMessage(tampered.dump(), localChain, pool));
  EXPECT_EQ(localChain.getChain().size(), 1u);
}

TEST_F(P2pServerTest, AcceptsIncomingBlockAndClearsMinedPoolTransactions) {
  Blockchain localChain;
  Blockchain remoteChain;
  TxnPool pool;
  Wallet wallet;
  P2pServer p2p;
  Miner localMiner(localChain, pool, wallet);
  localMiner.mine();

  // align remote chain tip with local tip before adding new block
  ASSERT_TRUE(remoteChain.replaceBlockchain(localChain.getChain()));

  auto tx1 = makeSignedCurrencyTxn(wallet, "bob", 11.0);
  ASSERT_TRUE(pool.addTxn(tx1));
  ASSERT_EQ(pool.getTxn().size(), 1u);

  remoteChain.addBlock({tx1});
  const Block &remoteBlock = remoteChain.getLatestBlock();
  std::string msg = P2pServer::makeBlockMessage(remoteBlock);

  EXPECT_TRUE(p2p.onPeerMessage(msg, localChain, pool));
  EXPECT_EQ(localChain.getChain().size(), 3u);
  EXPECT_TRUE(pool.getTxn().empty());
}
