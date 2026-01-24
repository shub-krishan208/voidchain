#include "block.h"
#include "./utils/TimeUtils.h"
#include <cstdint>
#include "./utils/hashing.h"

Block::Block(const int64_t timestamp, const std::string lastHash,
             const std::string hash, const std::string data)
    : timestamp(timestamp), lastHash(lastHash), hash(hash), data(data) {}

Block Block::genesis() {
  return Block(getCurrentTime(), "------", "0x00000000", "");
}

Block Block::mineBlock(Block lastBlock, std::string data) {
  auto timestamp = getCurrentTime();
  return Block(timestamp, lastBlock.getHash(), Block::hashBlock(std::to_string(timestamp), lastBlock.getHash(), data), data);
}

std::string Block::hashBlock(std::string timestamp, std::string lastHash, std::string data){
  Hasher h;
  h.add(timestamp);
  h.add(lastHash);
  h.add(data);

  return h.finish();
}