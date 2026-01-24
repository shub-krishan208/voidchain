#include "./utils/TimeUtils.h"
#include "block.h"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using str = std::string;
void printBlock(str title, Block b) {
  std::cout << title << "\n"
            << "Timestamp: " << getFormattedTimestamp(b.getTimestamp()) << "\n"
            << "Last Hash: " << b.getLastHash() << "\n"
            << "Hash: " << b.getHash() << "\n"
            << "Data: " << b.getData() << std::endl;
}

Block RandomBlock() {
  return Block(getCurrentTime(), "0000000000000000000lastHashExample",
               "0000000000000000000hashExample", "Some block data");
}
int main() {
  Block block = RandomBlock();
  Block FirstBlock = Block::genesis();
  Block newBlock = Block::mineBlock(block, "blud what's with the data!?");
  printBlock("Genesis Block", FirstBlock);
  printBlock("Block 1", block);
  printBlock("Mined Block", newBlock);
  return 0;
}
