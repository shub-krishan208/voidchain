#include "../src/TxnPool.h"
#include "../src/chain.h"
#include "../src/core/State.h"
#include "../src/miner.h"
#include "../src/models/AssetTxn.h"
#include "../src/models/CurrencyTxn.h"
#include "../src/wallet.h"
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

TEST(StateTest, DerivesRewardAccumulationForBalances) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet minerWallet;
  Miner miner(blockchain, pool, minerWallet);

  miner.mine();
  miner.mine();

  DerivedState state = State::deriveFromChainOrThrow(blockchain.getChain());
  EXPECT_DOUBLE_EQ(State::getBalance(state, minerWallet.getAddressPem()), 100.0);
}

TEST(StateTest, RejectsInsufficientBalanceOnPoolAdmission) {
  Blockchain blockchain;
  Wallet sender;

  auto spend = makeSignedCurrencyTxn(sender, "bob", 10.0);
  StateValidationResult result =
      State::validatePoolAdmission(spend, blockchain.getChain(), {});

  EXPECT_FALSE(result.ok);
}

TEST(StateTest, DerivesAssetOwnershipAfterClaimAndTransfer) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet owner;
  Wallet recipient;
  Miner miner(blockchain, pool, owner);

  auto claim = makeSignedAssetTxn(owner, owner.getAddressPem(), "item-77", "epic");
  ASSERT_TRUE(pool.addTxn(claim));
  miner.mine();

  auto transfer = makeSignedAssetTxn(owner, recipient.getAddressPem(), "item-77",
                                     "epic-transfer");
  ASSERT_TRUE(pool.addTxn(transfer));
  miner.mine();

  DerivedState state = State::deriveFromChainOrThrow(blockchain.getChain());
  EXPECT_EQ(State::getOwner(state, "item-77"), recipient.getAddressPem());
}

TEST(StateTest, RejectsAssetTransferByNonOwner) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet owner;
  Wallet attacker;
  Wallet recipient;
  Miner miner(blockchain, pool, owner);

  auto claim = makeSignedAssetTxn(owner, owner.getAddressPem(), "item-99", "rare");
  ASSERT_TRUE(pool.addTxn(claim));
  miner.mine();

  auto forgedTransfer =
      makeSignedAssetTxn(attacker, recipient.getAddressPem(), "item-99", "forged");
  StateValidationResult result =
      State::validatePoolAdmission(forgedTransfer, blockchain.getChain(), {});

  EXPECT_FALSE(result.ok);
}

TEST(StateTest, RejectsChainWithNonCanonicalCoinbaseReward) {
  Blockchain blockchain;

  auto badReward = std::make_shared<CurrencyTxn>();
  badReward->id = "bad-reward";
  badReward->from = "COINBASE";
  badReward->to = "miner";
  badReward->amount = 60.0;
  badReward->signature = "COINBASE";

  blockchain.addBlock({badReward});

  StateValidationResult result = State::validateFullChain(blockchain.getChain());
  EXPECT_FALSE(result.ok);
}
