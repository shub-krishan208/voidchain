#include "chain.h"
#include <stdexcept>
Blockchain::Blockchain() {
  // initialize with the genesis block
  chain.push_back(Block::genesis());
}

void Blockchain::addBlock(const std::vector<std::shared_ptr<Txn>> txns) {
  std::lock_guard<std::mutex> lock(chainMutex);
  Block newTxn = Block::mineBlock(getLatestBlock(), txns);
  chain.push_back(newTxn);
}
const Block &Blockchain::getLatestBlock() { return chain.back(); }

bool Blockchain::isValidBlockchain(const std::vector<Block> &newchain) {
  // check if the first block is valid genesis block
  const Block &genesis = Block::genesis();
  const Block &fblock = newchain[0];
  if (newchain.empty() || fblock.getHash() != genesis.getHash() ||
      fblock.getLastHash() != genesis.getLastHash() ||
      fblock.getMerkleRoot() != genesis.getMerkleRoot() ||
      fblock.getTimestamp() != genesis.getTimestamp()) {
    return false;
  }
  // singlie block chain
  if (newchain.size() == 1) {
    return true;
  }
  // validate each block in the chain
  for (size_t i = 1; i < newchain.size(); ++i) {
    const Block &currentBlock = newchain[i];
    const Block &previousBlock = newchain[i - 1];

    if (currentBlock.getLastHash() != previousBlock.getHash()) {
      return false;
    }

    // verify the hash of the current block
    std::string recalculatedHash = Block::hashBlock(
        // TODO: check to_string is system independent
        std::to_string(currentBlock.getTimestamp()), currentBlock.getLastHash(),
        currentBlock.getMerkleRoot());

    if (currentBlock.getHash() != recalculatedHash) {
      return false;
    }
  }

  return true;
}

bool Blockchain::replaceBlockchain(const std::vector<Block> &newchain) {
  std::lock_guard<std::mutex> lock(chainMutex);
  if (newchain.size() <= chain.size()) {
    throw std::invalid_argument(
        "Received chain is not longer than the current chain.");
  return false;
}
  if (!isValidBlockchain(newchain)) {
    throw std::invalid_argument("Received chain is invalid.");
  return false;
  }
  chain = newchain;
  return true;
}
