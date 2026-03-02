#include "miner.h"
#include "TxnPool.h"
#include "block.h"
#include "chain.h"

Miner::Miner(Blockchain &chain, TxnPool &pool) : chain_(chain), pool_(pool) {}

Block Miner::mine() {
  auto txns = pool_.getTxn();

  Block lastBlock = chain_.getLatestBlock();
  // Block newBlock = Block::mineBlock(lastBlock, txns);

  Block newBlock = chain_.addBlock(txns);

  pool_.clear(); // clear the txn pool

  return newBlock;
}