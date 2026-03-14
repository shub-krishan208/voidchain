#include "miner.h"
#include "TxnPool.h"
#include "block.h"
#include "chain.h"
#include "models/CurrencyTxn.h"
#include "utils/uuid.h"
#include <memory>

Miner::Miner(Blockchain &chain, TxnPool &pool, Wallet &wallet)
    : chain_(chain), pool_(pool), wallet_(wallet) {}

Block Miner::mine() {
  auto pendingTxns = pool_.getTxn();

  auto rewardTxn = std::make_shared<CurrencyTxn>();
  rewardTxn->id = generateUUID();
  rewardTxn->from = "COINBASE";
  rewardTxn->to = wallet_.getAddressPem();
  rewardTxn->amount = MINING_REWARD;
  rewardTxn->signature = "COINBASE";

  std::vector<std::shared_ptr<Txn>> txns;
  txns.reserve(pendingTxns.size() + 1);
  txns.push_back(rewardTxn);
  txns.insert(txns.end(), pendingTxns.begin(), pendingTxns.end());

  Block newBlock = chain_.addBlock(txns);

  for (const auto &txn : pendingTxns) {
    pool_.remove(txn->id);
  }

  return newBlock;
}