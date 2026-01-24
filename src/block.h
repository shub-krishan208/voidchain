#ifndef VOID_BLOCK_H
#define VOID_BLOCK_H
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

class Block {
public:
  Block(const int64_t timestamp, const std::string lastHash,
        const std::string hash, const std::string data);
  const int64_t &getTimestamp() const { return timestamp; };
  const std::string &getLastHash() const { return lastHash; };
  const std::string &getHash() const { return hash; };
  const std::string &getData() const { return data; };
  static Block genesis();
  static Block mineBlock(Block lastBlock, std::string data);
  static std::string hashBlock(std::string timestamp, std::string lastHash, std::string data);
private:
  int64_t timestamp;
  std::string lastHash;
  std::string hash;
  std::string data;
};

#endif // VOID_BLOCK_H
