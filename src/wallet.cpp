#include "wallet.h"
#include "utils/hex.h"
#include "utils/uuid.h"
#include <stdexcept>

Wallet::Wallet() { keyPair_ = OpenSSLWrapper::generateKeyPair(); }

Wallet::~Wallet() { EVP_PKEY_free(keyPair_); }

EVP_PKEY *Wallet::getPublicKey() const {
  return OpenSSLWrapper::extractPublicKey(keyPair_);
}

std::string Wallet::getAddress() const {
  EVP_PKEY *pub = getPublicKey();
  if (!pub) {
    throw std::runtime_error("Error: failed to derive public key");
  }

  std::string address = OpenSSLWrapper::publicKeyToAddress(pub);
  EVP_PKEY_free(pub);
  return address;
}

std::string Wallet::getPublicKeyHex() const {
  EVP_PKEY *pub = getPublicKey();
  if (!pub) {
    throw std::runtime_error("Error: failed to derive public key");
  }

  std::string publicKeyHex = OpenSSLWrapper::publicKeyToRawHex(pub);
  EVP_PKEY_free(pub);
  return publicKeyHex;
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

  const std::string address = getAddress();
  if (txn.from.empty()) {
    txn.from = address;
  } else if (txn.from != address) {
    throw std::runtime_error(
        "Error: txn sender does not match wallet identity");
  }

  const std::string senderPubKey = getPublicKeyHex();
  if (txn.senderPubKey.empty()) {
    txn.senderPubKey = senderPubKey;
  } else if (txn.senderPubKey != senderPubKey) {
    throw std::runtime_error(
        "Error: txn sender public key does not match wallet identity");
  }

  auto signableJson = txn.toSignableJson();
  auto sigBytes = sign(signableJson.dump());
  txn.signature = HexUtils::toHex(sigBytes);
}