#include "State.h"
#include "../TxnPool.h"
#include "../models/AssetTxn.h"
#include "../models/CurrencyTxn.h"
#include <cctype>
#include <cmath>
#include <stdexcept>

namespace {
constexpr double kFloatEpsilon = 1e-9;

std::string normalizeAddress(std::string value) {
  std::string normalized;
  normalized.reserve(value.size());

  for (size_t i = 0; i < value.size(); ++i) {
    const char current = value[i];

    if (current == '\\' && (i + 1) < value.size()) {
      const char next = value[i + 1];
      if (next == 'n') {
        normalized.push_back('\n');
        ++i;
        continue;
      }
      if (next == 'r') {
        if ((i + 3) < value.size() && value[i + 2] == '\\' &&
            value[i + 3] == 'n') {
          normalized.push_back('\n');
          i += 3;
          continue;
        }
        normalized.push_back('\n');
        ++i;
        continue;
      }
    }

    if (current == '\r') {
      if ((i + 1) < value.size() && value[i + 1] == '\n') {
        ++i;
      }
      normalized.push_back('\n');
      continue;
    }

    normalized.push_back(current);
  }

  size_t start = 0;
  while (start < normalized.size() &&
         std::isspace(static_cast<unsigned char>(normalized[start])) != 0) {
    ++start;
  }

  size_t end = normalized.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(normalized[end - 1])) != 0) {
    --end;
  }

  return normalized.substr(start, end - start);
}
}

StateValidationResult State::deriveFromChain(const std::vector<Block> &chain,
                                             DerivedState &outState) {
  outState = DerivedState{};
  if (chain.empty()) {
    return StateValidationResult::failure("Cannot derive state from empty chain");
  }

  for (size_t i = 1; i < chain.size(); ++i) {
    const auto &txs = chain[i].getTransactions();
    size_t coinbaseCount = 0;
    for (const auto &txn : txs) {
      if (!TxnPool::verifyTxnSignatureAndFormat(txn)) {
        return StateValidationResult::failure(
            "Block contains invalid transaction signature/format");
      }
      if (txn->from == COINBASE) {
        ++coinbaseCount;
      }
      StateValidationResult applyResult = applyTxn(txn, outState);
      if (!applyResult.ok) {
        return applyResult;
      }
    }
    if (coinbaseCount > 1) {
      return StateValidationResult::failure(
          "Block cannot contain multiple coinbase transactions");
    }
  }

  return StateValidationResult::success();
}

DerivedState State::deriveFromChainOrThrow(const std::vector<Block> &chain) {
  DerivedState state;
  StateValidationResult result = deriveFromChain(chain, state);
  if (!result.ok) {
    throw std::runtime_error(result.error);
  }
  return state;
}

StateValidationResult State::validatePoolAdmission(
    const std::shared_ptr<Txn> &candidate, const std::vector<Block> &chain,
    const std::vector<std::shared_ptr<Txn>> &pendingPool) {
  if (!TxnPool::verifyTxnSignatureAndFormat(candidate)) {
    return StateValidationResult::failure("Transaction signature/format invalid");
  }

  if (candidate->from == COINBASE) {
    return StateValidationResult::failure(
        "Coinbase transactions cannot be added to the pool");
  }

  DerivedState state;
  StateValidationResult deriveResult = deriveFromChain(chain, state);
  if (!deriveResult.ok) {
    return deriveResult;
  }

  if (auto currency = std::dynamic_pointer_cast<CurrencyTxn>(candidate)) {
    const std::string candidateFrom = normalizeAddress(candidate->from);
    double pendingOutflow = 0.0;
    for (const auto &pending : pendingPool) {
      if (!pending || normalizeAddress(pending->from) != candidateFrom ||
          pending->from == COINBASE) {
        continue;
      }
      auto pendingCurrency = std::dynamic_pointer_cast<CurrencyTxn>(pending);
      if (!pendingCurrency || pendingCurrency->amount <= 0.0) {
        continue;
      }
      pendingOutflow += pendingCurrency->amount;
    }

    const double confirmedBalance = getBalance(state, candidateFrom);
    const double availableBalance = confirmedBalance - pendingOutflow;
    if (availableBalance + kFloatEpsilon < currency->amount) {
      return StateValidationResult::failure(
          "Insufficient balance after accounting for pending spends");
    }
  }

  if (auto asset = std::dynamic_pointer_cast<AssetTxn>(candidate)) {
    const std::string candidateFrom = normalizeAddress(asset->from);
    std::map<std::string, std::string> effectiveOwner = state.ownerByAsset;
    for (const auto &pending : pendingPool) {
      auto pendingAsset = std::dynamic_pointer_cast<AssetTxn>(pending);
      if (!pendingAsset) {
        continue;
      }

      auto ownerIt = effectiveOwner.find(pendingAsset->itemId);
      const std::string pendingFrom = normalizeAddress(pendingAsset->from);
      if (ownerIt != effectiveOwner.end() && ownerIt->second == pendingFrom) {
        effectiveOwner[pendingAsset->itemId] =
            normalizeAddress(pendingAsset->to);
      }
    }

    auto ownerIt = effectiveOwner.find(asset->itemId);
    if (ownerIt == effectiveOwner.end()) {
      if (candidateFrom != normalizeAddress(asset->to)) {
        return StateValidationResult::failure(
            "Asset transfer rejected: unowned assets require self-claim");
      }
      return StateValidationResult::success();
    }

    if (ownerIt->second != candidateFrom) {
      return StateValidationResult::failure(
          "Asset transfer rejected: sender is not current owner");
    }
  }

  return StateValidationResult::success();
}

StateValidationResult
State::validateBlockAppend(const Block &block,
                           const std::vector<Block> &currentChain) {
  DerivedState state;
  StateValidationResult deriveResult = deriveFromChain(currentChain, state);
  if (!deriveResult.ok) {
    return deriveResult;
  }

  size_t coinbaseCount = 0;
  for (const auto &txn : block.getTransactions()) {
    if (!TxnPool::verifyTxnSignatureAndFormat(txn)) {
      return StateValidationResult::failure(
          "Incoming block contains invalid transaction signature/format");
    }
    if (txn->from == COINBASE) {
      ++coinbaseCount;
    }

    StateValidationResult applyResult = applyTxn(txn, state);
    if (!applyResult.ok) {
      return applyResult;
    }
  }

  if (coinbaseCount > 1) {
    return StateValidationResult::failure(
        "Incoming block contains multiple coinbase transactions");
  }

  return StateValidationResult::success();
}

StateValidationResult State::validateFullChain(const std::vector<Block> &chain) {
  DerivedState state;
  return deriveFromChain(chain, state);
}

double State::getBalance(const DerivedState &state, const std::string &address) {
  auto it = state.balances.find(normalizeAddress(address));
  if (it == state.balances.end()) {
    return 0.0;
  }
  return it->second;
}

std::set<std::string> State::getAssets(const DerivedState &state,
                                       const std::string &address) {
  auto it = state.assetsByOwner.find(normalizeAddress(address));
  if (it == state.assetsByOwner.end()) {
    return {};
  }
  return it->second;
}

std::string State::getOwner(const DerivedState &state,
                            const std::string &itemId) {
  auto it = state.ownerByAsset.find(itemId);
  if (it == state.ownerByAsset.end()) {
    return "";
  }
  return it->second;
}

StateValidationResult State::applyTxn(const std::shared_ptr<Txn> &txn,
                                      DerivedState &state) {
  if (!txn) {
    return StateValidationResult::failure("Null transaction");
  }

  auto currency = std::dynamic_pointer_cast<CurrencyTxn>(txn);
  if (currency) {
    const std::string toAddress = normalizeAddress(currency->to);
    if (toAddress.empty() || currency->amount <= 0.0) {
      return StateValidationResult::failure(
          "Currency transaction must have positive amount and recipient");
    }

    if (currency->from == COINBASE) {
      if (std::fabs(currency->amount - MINING_REWARD) > kFloatEpsilon) {
        return StateValidationResult::failure(
            "Coinbase transaction amount must match canonical mining reward");
      }
      state.balances[toAddress] += currency->amount;
      return StateValidationResult::success();
    }

    const std::string fromAddress = normalizeAddress(currency->from);
    if (fromAddress.empty()) {
      return StateValidationResult::failure(
          "Currency transaction sender cannot be empty");
    }

    const double senderBalance = getBalance(state, fromAddress);
    if (senderBalance + kFloatEpsilon < currency->amount) {
      return StateValidationResult::failure(
          "Currency transaction rejected: insufficient balance");
    }

    state.balances[fromAddress] = senderBalance - currency->amount;
    state.balances[toAddress] += currency->amount;
    return StateValidationResult::success();
  }

  auto asset = std::dynamic_pointer_cast<AssetTxn>(txn);
  if (asset) {
    const std::string fromAddress = normalizeAddress(asset->from);
    const std::string toAddress = normalizeAddress(asset->to);
    if (fromAddress.empty() || toAddress.empty() || asset->itemId.empty()) {
      return StateValidationResult::failure(
          "Asset transaction must include sender, recipient, and itemId");
    }

    auto ownerIt = state.ownerByAsset.find(asset->itemId);
    if (ownerIt == state.ownerByAsset.end()) {
      if (fromAddress != toAddress) {
        return StateValidationResult::failure(
            "Asset transaction rejected: unowned assets require self-claim");
      }
      state.ownerByAsset[asset->itemId] = fromAddress;
      state.assetsByOwner[fromAddress].insert(asset->itemId);
      return StateValidationResult::success();
    }

    if (ownerIt->second != fromAddress) {
      return StateValidationResult::failure(
          "Asset transaction rejected: sender is not current owner");
    }

    state.ownerByAsset[asset->itemId] = toAddress;

    auto fromAssetsIt = state.assetsByOwner.find(fromAddress);
    if (fromAssetsIt != state.assetsByOwner.end()) {
      fromAssetsIt->second.erase(asset->itemId);
    }
    state.assetsByOwner[toAddress].insert(asset->itemId);
    return StateValidationResult::success();
  }

  return StateValidationResult::failure("Unknown transaction type");
}
