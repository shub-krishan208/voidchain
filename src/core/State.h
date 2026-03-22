#pragma once

#include "../block.h"
#include "../models/Txn.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct DerivedState {
  std::map<std::string, double> balances;
  std::map<std::string, std::string> ownerByAsset;
  std::map<std::string, std::set<std::string>> assetsByOwner;
};

struct StateValidationResult {
  bool ok{false};
  std::string error;

  static StateValidationResult success() { return {true, ""}; }
  static StateValidationResult failure(const std::string &message) {
    return {false, message};
  }
};

class State {
public:
  static constexpr const char *COINBASE = "COINBASE";
  static constexpr double MINING_REWARD = 50.0;

  static StateValidationResult deriveFromChain(const std::vector<Block> &chain,
                                               DerivedState &outState);
  static DerivedState deriveFromChainOrThrow(const std::vector<Block> &chain);

  static StateValidationResult
  validatePoolAdmission(const std::shared_ptr<Txn> &candidate,
                        const std::vector<Block> &chain,
                        const std::vector<std::shared_ptr<Txn>> &pendingPool);

  static StateValidationResult
  validateBlockAppend(const Block &block,
                      const std::vector<Block> &currentChain);

  static StateValidationResult validateFullChain(const std::vector<Block> &chain);

  static double getBalance(const DerivedState &state, const std::string &address);
  static std::set<std::string> getAssets(const DerivedState &state,
                                         const std::string &address);
  static std::string getOwner(const DerivedState &state,
                              const std::string &itemId);

private:
  static StateValidationResult applyTxn(const std::shared_ptr<Txn> &txn,
                                        DerivedState &state);
};
