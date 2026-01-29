#include "./utils/TimeUtils.h"
#include "block.h"
#include "chain.h"
#include "router.h"

#include <iostream>
// #include <nlohmann/json.hpp>

// using json = nlohmann::json;
using str = std::string;
void printBlock(str title, Block b) {
  std::cout << title << "\n"
            << "Timestamp: " << getFormattedTimestamp(b.getTimestamp()) << "\n"
            << "Last Hash: " << b.getLastHash() << "\n"
            << "Hash: " << b.getHash() << "\n"
            << "Data: " << b.getData() << std::endl;
}

void printBlocks() {
  Block FirstBlock = Block::genesis();
  Block newBlock = Block::mineBlock(FirstBlock, "blud what's with the data!?");
  Block block = Block::mineBlock(newBlock, "Another Block Data");
  printBlock("Genesis Block", FirstBlock);
  printBlock("Random Block", block);
  printBlock("Mined Block", newBlock);
}

void printChain() {
  Blockchain chain;
  printBlock("Chain initialised", chain.getLatestBlock());
  chain.addBlock("First added block");
  printBlock("After adding first block", chain.getLatestBlock());
}

int main() {
  Blockchain bc;
  crow::SimpleApp app;

  Router router(app, bc);

  router.registerRoutes();
  printChain();

  app.port(18169).multithreaded().run();
  // printBlocks();
  return 0;
}
