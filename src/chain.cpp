#include "chain.h"
#include "block.h"
#include <stdexcept>
Blockchain::Blockchain() {
  // initialize with the genesis block
  chain.push_back(Block::genesis());
}

Block Blockchain::addBlock(const std::vector<std::shared_ptr<Txn>> txns) {
  std::lock_guard<std::mutex> lock(chainMutex);
  Block newBlock = Block::mineBlock(getLatestBlock(), txns);
  if (isValidBlock(newBlock, getLatestBlock())) {
    chain.push_back(newBlock);
  } else {
    throw std::runtime_error("Mined block is invalid");
  }
  return newBlock;
}

bool Blockchain::addBlock(const Block &block) {
  std::lock_guard<std::mutex> lock(chainMutex);
  if (!isValidBlock(block, getLatestBlock())) {
    return false;
  }
  chain.push_back(block);
  return true;
}

bool Blockchain::isValidBlock(const Block &block, const Block &previousBlock) {
  if (block.getLastHash() != previousBlock.getHash()) {
    return false;
  }
  std::string recalculatedHash =
      Block::hashBlock(std::to_string(block.getTimestamp()),
                       block.getLastHash(), block.getMerkleRoot());
  if (block.getHash() != recalculatedHash) {
    return false;
  }
  return true;
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
    if (!isValidBlock(newchain[i], newchain[i - 1])) {
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
