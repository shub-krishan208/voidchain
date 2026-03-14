#include "miner.h"
#include "TxnPool.h"
#include "block.h"
#include "chain.h"
#include <stdexcept>

Miner::Miner(Blockchain &chain, TxnPool &pool) : chain_(chain), pool_(pool) {}

Block Miner::mine() {
  auto txns = pool_.getTxn();
  if (txns.empty()) {
    throw std::runtime_error("Error: no transactions in pool to mine");
  }

  Block newBlock = chain_.addBlock(txns);

  for (const auto &txn : txns) {
    pool_.remove(txn->id);
  }

  return newBlock;
}