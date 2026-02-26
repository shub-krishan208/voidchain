#include "wallet.h"
#include "utils/hex.h"
#include "utils/uuid.h"

Wallet::Wallet() { keyPair_ = OpenSSLWrapper::generateKeyPair(); }

Wallet::~Wallet() { EVP_PKEY_free(keyPair_); }

EVP_PKEY *Wallet::getAddress() const { return keyPair_; }

EVP_PKEY *Wallet::getPublicKey() const {
  return OpenSSLWrapper::extractPublicKey(keyPair_);
}

std::vector<unsigned char> Wallet::sign(const std::string &data) const {
  return OpenSSLWrapper::sign(keyPair_, data);
}

void Wallet::signTxn(Txn &txn) const {
  if (!txn.signature.empty()) {
    throw std::runtime_error("Error: Transaction already signed");
  }

  if (txn.id.empty()) {
    txn.id = generateUUID();
  }
  auto signableJson = txn.toSignableJson();
  auto sigBytes = sign(signableJson.dump());
  txn.signature = HexUtils::toHex(sigBytes);
}