#include "TxnPool.h"
#include "models/CurrencyTxn.h"
#include "osslWrapper.h"
#include "utils/hex.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <memory>
#include <vector>

namespace {
std::string normalizeAddress(std::string value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    ++start;
  }

  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }

  std::string trimmed = value.substr(start, end - start);
  if (trimmed.empty()) {
    return {};
  }

  bool hadPrefix = false;
  if (trimmed.size() >= 2 && trimmed[0] == '0' &&
      (trimmed[1] == 'x' || trimmed[1] == 'X')) {
    trimmed.erase(0, 2);
    hadPrefix = true;
  }

  std::string lowered = trimmed;
  std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  const bool isHexBody =
      !lowered.empty() &&
      std::all_of(lowered.begin(), lowered.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
      });
  if (hadPrefix || (isHexBody && lowered.size() == 40)) {
    return "0x" + lowered;
  }
  return trimmed;
}
} // namespace

bool TxnPool::addTxn(std::shared_ptr<Txn> txn) {
  if (!txn)
    return false;

  if (!verifyTxn(txn)) {
    return false;
  }

  pool_[txn->id] = txn;
  return true;
}

bool TxnPool::verifyTxn(const std::shared_ptr<Txn> &txn) const {
  if (!txn)
    return false;
  if (pool_.find(txn->id) != pool_.end())
    return false;
  return verifyTxnSignatureAndFormat(txn);
}

bool TxnPool::verifyTxnSignatureAndFormat(const std::shared_ptr<Txn> &txn) {
  if (!txn)
    return false;
  if (txn->id.empty())
    return false;

  if (txn->from == "COINBASE") {
    auto rewardTxn = std::dynamic_pointer_cast<CurrencyTxn>(txn);
    if (!rewardTxn)
      return false;
    return !rewardTxn->to.empty() && rewardTxn->amount > 0.0;
  }

  if (txn->signature.empty())
    return false;
  if (txn->from.empty())
    return false;
  if (txn->senderPubKey.empty())
    return false;

  EVP_PKEY *pubkey = OpenSSLWrapper::publicKeyFromRawHex(txn->senderPubKey);
  if (!pubkey) {
    return false;
  }

  const std::string derivedAddress = OpenSSLWrapper::publicKeyToAddress(pubkey);
  if (derivedAddress.empty() ||
      normalizeAddress(derivedAddress) != normalizeAddress(txn->from)) {
    EVP_PKEY_free(pubkey);
    return false;
  }

  std::vector<unsigned char> sigBytes;
  try {
    sigBytes = HexUtils::fromHex(txn->signature);
  } catch (const std::exception &) {
    EVP_PKEY_free(pubkey);
    return false;
  }
  std::string data = txn->toSignableJson().dump();

  bool valid = OpenSSLWrapper::verify(pubkey, data, sigBytes);
  EVP_PKEY_free(pubkey);
  return valid;
}

std::vector<std::shared_ptr<Txn>> TxnPool::getTxn() const {
  std::vector<std::shared_ptr<Txn>> res;
  for (const auto &[id, txn] : pool_)
    res.push_back(txn);
  return res;
}

void TxnPool::remove(const std::string &id) { pool_.erase(id); }

void TxnPool::clear() { pool_.clear(); }