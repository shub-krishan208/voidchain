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
  Block(const int64_t timestamp, 
        const std::string lastHash,
        const std::string hash, 
        const std::string merkleRoot, 
        const std::vector<std::shared_ptr<Txn>>& transactions);
  
        const int64_t &getTimestamp() const { return timestamp; };
  const std::string &getLastHash() const { return lastHash; };
  const std::string &getHash() const { return hash; };
  const std::string &getMerkleRoot() const { return merkleRoot; };
  static Block genesis();
  static Block mineBlock(Block lastBlock, std::vector<std::shared_ptr<Txn>> transactions);
  static std::string hashBlock(std::string timestamp, std::string lastHash,
                               std::string merkleRoot);

  crow::json::wvalue toJson() const;
  static Block fromJson(const crow::json::rvalue &json);

private:
  int64_t timestamp;
  std::string lastHash;
  std::string hash;

  std::vector<std::shared_ptr<Txn>> transactions;
  std::string merkleRoot;
};

#endif // VOID_BLOCK_H
