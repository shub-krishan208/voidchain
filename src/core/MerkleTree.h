#pragma once
#include "../models/Txn.h"
#include "../utils/hashing.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class MerkleTree {
public:
  explicit MerkleTree(const std::vector<std::shared_ptr<Txn>> &txs) {
    buildTree(txs);
  }

  struct ProofNode{
    std::string hash;
    bool isLeft;
  };

  const std::string &getRoot() const { return root; }


  /**
   * 
   * @param txHash 
   * @return Path of sibling hashes from txn leaf to root
   * TODO make a proofNode struct with is left boolean to preserve ordering ....
   */
  std::vector<ProofNode> getProof(const std::string &txHash) const {
    std::vector<ProofNode> proof;
    if (levels.empty()) {
      return proof;
    }

    // find leaf index
    const auto& leaves = levels[0];
    auto it = std::find(leaves.begin(), leaves.end(), txHash);

    if (it == leaves.end()) {
    return proof; // txn not found
    }
    

    size_t idx = std::distance(leaves.begin(), it);
    bool isLeft;
    //climb up the tree
    for (size_t level=0; level < levels.size()-1; level++) {
      const auto& currentLevel = levels[level];

      size_t siblingIdx;

      if (idx % 2 == 0) {
        siblingIdx = idx + 1;
        if (siblingIdx >= currentLevel.size()) {
        siblingIdx = idx;
        }
        isLeft = false;
      } else {
        siblingIdx = idx - 1;
        isLeft = true;
      }

      proof.push_back({ currentLevel[siblingIdx], isLeft });
      idx /= 2;
    }
    
    return proof;
  }
  
  /**
   * 
   * @param root 
   * @param txHash 
   * @param proof 
   * @return Bool for verification
   */
  static bool verifyProof(
    const std::string& root,
    const std::string& txHash,
    const std::vector<ProofNode>& proof){
   
      Hasher h;
      std::string current = txHash;

      for (const auto &node : proof) {
        Hasher k;
        if(node.isLeft){
          k.add(node.hash);
          k.add(current);
        } else {
          k.add(current);
          k.add(node.hash);
        }
        current = k.finish();
      }
      return current == root;
  }

private:
  std::string root;
  std::vector<std::vector<std::string>> levels;

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
    levels.clear();

    if (txs.empty()) {
      root = "";
      return;
    }

    //build leaves
    std::vector<std::string> level;

    for (const auto &tx : txs)
      level.push_back(hash(tx->toJson().dump()));

    levels.push_back(level); // leaves are the txn hashes

    //build upwards
    while (level.size() > 1) {
      if (level.size() % 2 != 0)
        level.push_back(level.back());

      std::vector<std::string> next;

      for (size_t i = 0; i < level.size(); i += 2) {
        next.push_back(hashPair(level[i], level[i + 1]));
      }
      levels.push_back(next);
      level = next;
    }

    root = levels.back()[0];
  }
};
