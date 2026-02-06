#include "block.h"

Block::Block(const int64_t timestamp, const std::string lastHash,
             const std::string hash, const std::string data)
    : timestamp(timestamp), lastHash(lastHash), hash(hash), data(data) {}

Block Block::genesis() {
  std::string prevGenesisHash(64, '0');
  const int64_t GENESIS_TIMESTAMP = 1769351684715; // Jan 24, 2026
  return Block(GENESIS_TIMESTAMP, prevGenesisHash, Block::hashBlock(std::to_string(GENESIS_TIMESTAMP), prevGenesisHash, ""), "");
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

crow::json::wvalue Block::toJson() const {
  crow::json::wvalue json;
  json["timestamp"] = timestamp;
  json["last_hash"] = lastHash;
  json["hash"] = hash;
  json["data"] = data;
  return json;
}

Block Block::fromJson(const crow::json::rvalue &json) {
  int64_t timestamp = json["timestamp"].i();
  std::string lastHash = json["last_hash"].s();
  std::string hash = json["hash"].s();
  std::string data = json["data"].s();
  return Block(timestamp, lastHash, hash, data);
}