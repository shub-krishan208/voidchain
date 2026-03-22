#ifndef VOID_BLOCK_H
#define VOID_BLOCK_H
#include "./utils/TimeUtils.h"
#include "./utils/hashing.h"
#include "./core/MerkleTree.h"

#include <cstdint>

#include <crow/json.h>
#include <cstdint>
// #include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>

class Block {
public:
  static constexpr int INITIAL_DIFFICULTY = 2;
  static constexpr int MINE_RATE_MS = 3000;

  Block(const int64_t timestamp, const std::string lastHash,
        const std::string hash, const std::string merkleRoot,
        const std::vector<std::shared_ptr<Txn>> &transactions, const int nonce,
        const int difficulty);

  const int64_t &getTimestamp() const { return timestamp; };
  const std::string &getLastHash() const { return lastHash; };
  const std::string &getHash() const { return hash; };
  const std::string &getMerkleRoot() const { return merkleRoot; };
  const std::vector<std::shared_ptr<Txn>> &getTransactions() const {
    return transactions;
  };
  int getNonce() const { return nonce; };
  int getDifficulty() const { return difficulty; };
  static Block genesis();
  static Block
  mineBlock(const Block &lastBlock, std::vector<std::shared_ptr<Txn>> txns);
  static std::string hashBlock(const std::string &timestamp,
                               const std::string &lastHash,
                               const std::string &merkleRoot, const int nonce,
                               const int difficulty);
  static int adjustDifficulty(const Block &lastBlock, int64_t currentTime);

  crow::json::wvalue headerToJson() const;
  crow::json::wvalue toJson() const;
  static Block fromJson(const crow::json::rvalue &json);

private:
  int64_t timestamp;
  std::string lastHash;
  std::string hash;

  std::vector<std::shared_ptr<Txn>> transactions;
  std::string merkleRoot;
  int nonce;
  int difficulty;
};

#endif // VOID_BLOCK_H
