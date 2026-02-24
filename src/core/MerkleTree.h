#pragma once
#include "../models/Txn.h"
#include "../utils/hashing.h"
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class MerkleTree {
public:
  explicit MerkleTree(const std::vector<std::shared_ptr<Txn>> &txs) {
    buildTree(txs);
  }

  const std::string &getRoot() const { return root; }

private:
  std::string root;

  std::string hash(const std::string &data) {
    Hasher h;
    h.add(data);
    return h.finish();
  }
  std::string hashPair(const std::string &a, const std::string &b) {
    Hasher h;
    h.add(a);
    h.add(b);
    return h.finish();
  }
  void buildTree(const std::vector<std::shared_ptr<Txn>> &txs) {
    if (txs.empty()) {
      root = "";
      return;
    }

    std::vector<std::string> level;

    for (const auto &tx : txs)
      level.push_back(hash(tx->toJson().dump()));

    while (level.size() > 1) {
      if (level.size() % 2 != 0)
        level.push_back(level.back());

      std::vector<std::string> next;

      for (size_t i = 0; i < level.size(); i += 2) {
        next.push_back(hashPair(level[i], level[i + 1]));
      }
      level = next;
    }

    root = level[0];
  }
};
