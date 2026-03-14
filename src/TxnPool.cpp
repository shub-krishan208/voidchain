#include "TxnPool.h"
#include "models/CurrencyTxn.h"
#include "osslWrapper.h"
#include "utils/hex.h"

#include <memory>
#include <openssl/bio.h>
#include <openssl/ec.h>
#include <openssl/pem.h>
#include <vector>

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

  // reconstruct pubkey from PEM string
  BIO *bio = BIO_new_mem_buf(txn->from.data(), txn->from.size());
  if (!bio)
    return false;

  EVP_PKEY *pubkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
  BIO_free(bio);

  if (!pubkey)
    return false;

  // convert hex signature back to raw bytes
  std::vector<unsigned char> sigBytes = HexUtils::fromHex(txn->signature);
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