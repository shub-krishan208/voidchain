#include "chain.h"
#include "block.h"
#include <cstdlib>
#include <string>
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

  if (block.getDifficulty() < 1) {
    return false;
  }

  if (std::abs(block.getDifficulty() - previousBlock.getDifficulty()) > 1) {
    return false;
  }

  std::string recalculatedHash =
      Block::hashBlock(std::to_string(block.getTimestamp()),
                       block.getLastHash(), block.getMerkleRoot(),
                       block.getNonce(), block.getDifficulty());
  if (block.getHash() != recalculatedHash) {
    return false;
  }

  const std::string target(static_cast<size_t>(block.getDifficulty()), '0');
  if (block.getHash().substr(0, static_cast<size_t>(block.getDifficulty())) !=
      target) {
    return false;
  }

  return true;
}

const Block &Blockchain::getLatestBlock() { return chain.back(); }

bool Blockchain::isValidBlockchain(const std::vector<Block> &newchain) {
  if (newchain.empty()) {
    return false;
  }

  // check if the first block is valid genesis block
  const Block &genesis = Block::genesis();
  const Block &fblock = newchain[0];
  if (fblock.getHash() != genesis.getHash() ||
      fblock.getLastHash() != genesis.getLastHash() ||
      fblock.getMerkleRoot() != genesis.getMerkleRoot() ||
      fblock.getTimestamp() != genesis.getTimestamp() ||
      fblock.getDifficulty() != genesis.getDifficulty() ||
      fblock.getNonce() != genesis.getNonce()) {
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
