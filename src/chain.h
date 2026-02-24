#ifndef VOID_CHAIN_H
#define VOID_CHAIN_H

#include "./utils/hashing.h"
#include "models/Txn.h"
#include "block.h"
#include <vector>
#include <mutex>
class Blockchain {
public:
  Blockchain();
  std::mutex chainMutex;

  // create the block and add to the chain. (mining logic implemented later)
  const std::vector<Block> &getChain() const { return chain; };
  void addBlock(const std::vector<std::shared_ptr<Txn>> txns);
  const Block &getLatestBlock(); // outputs the block at the last of the chain
  static bool isValidBlockchain(const std::vector<Block> &newchain);
  bool replaceBlockchain(const std::vector<Block> &newBlockchain);

private:
  std::vector<Block> chain;
};
#endif // VOID_CHAIN_H
