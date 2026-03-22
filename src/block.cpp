#include "block.h"
#include "core/MerkleTree.h"
#include "core/TxnFactory.h"
#include "utils/TimeUtils.h"
#include "utils/hashing.h"
#include <cstddef>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <stdexcept>

Block::Block(const int64_t timestamp, const std::string lastHash,
             const std::string hash, const std::string merkleRoot,
             const std::vector<std::shared_ptr<Txn>> &txs, const int nonce,
             const int difficulty)
    : timestamp(timestamp), lastHash(lastHash), hash(hash),
      merkleRoot(merkleRoot), transactions(txs), nonce(nonce),
      difficulty(difficulty) {}

Block Block::genesis() {
  std::string prevGenesisHash(64, '0');
  const int64_t GENESIS_TIMESTAMP = 1769351684715;
  const int nonce = 0;
  const int difficulty = INITIAL_DIFFICULTY;

  std::vector<std::shared_ptr<Txn>> emptyTxs;
  std::string root = "";

  std::string hash =
      Block::hashBlock(std::to_string(GENESIS_TIMESTAMP), prevGenesisHash, root,
                       nonce, difficulty);

  return Block(GENESIS_TIMESTAMP, prevGenesisHash, hash, root, emptyTxs, nonce,
               difficulty);
}

Block Block::mineBlock(const Block &lastBlock,
                       std::vector<std::shared_ptr<Txn>> txs) {
  int64_t timestamp = 0;
  int nonce = 0;
  int difficulty = lastBlock.getDifficulty();

  MerkleTree tree(txs);
  std::string root = tree.getRoot();
  std::string hash;

  do {
    nonce += 1;
    timestamp = getCurrentTime();
    difficulty = Block::adjustDifficulty(lastBlock, timestamp);
    hash = Block::hashBlock(std::to_string(timestamp), lastBlock.getHash(), root,
                            nonce, difficulty);
  } while (hash.substr(0, static_cast<size_t>(difficulty)) !=
           std::string(static_cast<size_t>(difficulty), '0'));

  return Block(timestamp, lastBlock.getHash(), hash, root, txs, nonce,
               difficulty);
}

std::string Block::hashBlock(const std::string &timestamp,
                             const std::string &lastHash,
                             const std::string &merkleRoot, const int nonce,
                             const int difficulty) {
  Hasher h;
  h.add(timestamp);
  h.add(lastHash);
  h.add(merkleRoot);
  h.add(std::to_string(nonce));
  h.add(std::to_string(difficulty));

  return h.finish();
}

int Block::adjustDifficulty(const Block &lastBlock, int64_t currentTime) {
  if (lastBlock.getTimestamp() + MINE_RATE_MS > currentTime) {
    return lastBlock.getDifficulty() + 1;
  }
  return std::max(1, lastBlock.getDifficulty() - 1);
}

crow::json::wvalue Block::headerToJson() const {
  crow::json::wvalue header;
  header["timestamp"] = timestamp;
  header["last_hash"] = lastHash;
  header["hash"] = hash;
  header["merkle_root"] = merkleRoot;
  header["nonce"] = nonce;
  header["difficulty"] = difficulty;
  return header;
}

crow::json::wvalue Block::toJson() const {

  crow::json::wvalue json = headerToJson();

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
  int nonce = json["nonce"].i();
  int difficulty = json["difficulty"].i();
  if (difficulty < 1) {
    throw std::runtime_error("Invalid block: difficulty must be >= 1");
  }

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

  return Block(timestamp, lastHash, hash, merkleRoot, txns, nonce, difficulty);
}
