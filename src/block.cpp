#include "block.h"
#include "core/MerkleTree.h"
#include "core/TxnFactory.h"
#include "utils/TimeUtils.h"
#include "utils/hashing.h"
#include <cstddef>
#include <nlohmann/json.hpp>

Block::Block(const int64_t timestamp, const std::string lastHash,
             const std::string hash, const std::string merkleRoot,
             const std::vector<std::shared_ptr<Txn>> &txs)
    : timestamp(timestamp), lastHash(lastHash), hash(hash),
      merkleRoot(merkleRoot), transactions(txs) {}

Block Block::genesis() {
  std::string prevGenesisHash(64, '0');
  const int64_t GENESIS_TIMESTAMP = 1769351684715;

  std::vector<std::shared_ptr<Txn>> emptyTxs;
  std::string root = "";

  std::string hash = Block::hashBlock(std::to_string(GENESIS_TIMESTAMP),
                                      prevGenesisHash, root);

  return Block(GENESIS_TIMESTAMP, prevGenesisHash, hash, root, emptyTxs);
}

Block Block::mineBlock(Block lastBlock, std::vector<std::shared_ptr<Txn>> txs) {

  int64_t timestamp = getCurrentTime();

  MerkleTree tree(txs);
  std::string root = tree.getRoot();

  std::string hash =
      Block::hashBlock(std::to_string(timestamp), lastBlock.getHash(), root);

  return Block(timestamp, lastBlock.getHash(), hash, root, txs);
}

std::string Block::hashBlock(std::string timestamp, std::string lastHash,
                             std::string merkleRoot) {

  Hasher h;
  h.add(timestamp);
  h.add(lastHash);
  h.add(merkleRoot);

  return h.finish();
}

crow::json::wvalue Block::toJson() const {

  crow::json::wvalue json;

  json["timestamp"] = timestamp;
  json["last_hash"] = lastHash;
  json["hash"] = hash;
  json["merkle_root"] = merkleRoot;

  std::vector<crow::json::wvalue> txList;

  for (size_t i = 0; i < transactions.size(); ++i) {
    std::string txStr = transactions[i]->toJson().dump();
    txList.push_back(crow::json::load(txStr));
  }

  json["transactions"] = std::move(txList);

  return json;
}

Block Block::fromJson(const crow::json::rvalue &json) {

  int64_t timestamp = json["timestamp"].i();
  std::string lastHash = json["last_hash"].s();
  std::string hash = json["hash"].s();
  std::string merkleRoot = json["merkle_root"].s();

  std::vector<std::shared_ptr<Txn>> txns;

  // transactions field sent over the network is an array of txns
  const auto &txArray = json["transactions"];

  for (size_t i = 0; i < txArray.size(); i++) {
    // convert crow::json::rvalue object to string then parse with
    // nlohmann::json
    std::string txStr = crow::json::wvalue(txArray[i]).dump();
    nlohmann::json j = nlohmann::json::parse(txStr);

    auto txn = TxnFactory::createTxn(j);
    txns.push_back(txn);
  }

  MerkleTree tree(txns);
  std::string computedRoot = tree.getRoot();

  if (computedRoot != merkleRoot) {
    throw std::runtime_error("Invalid block: Merkle root mismatch");
  }

  return Block(timestamp, lastHash, hash, merkleRoot, txns);
}
