#include "wallet.h"
#include "utils/hex.h"
#include "utils/uuid.h"
#include <stdexcept>

Wallet::Wallet() { keyPair_ = OpenSSLWrapper::generateKeyPair(); }

Wallet::~Wallet() { EVP_PKEY_free(keyPair_); }

EVP_PKEY *Wallet::getAddress() const { return keyPair_; }

EVP_PKEY *Wallet::getPublicKey() const {
  return OpenSSLWrapper::extractPublicKey(keyPair_);
}

std::string Wallet::getAddressPem() const {
  EVP_PKEY *pub = getPublicKey();
  if (!pub) {
    throw std::runtime_error("Error: failed to derive public key");
  }

  std::string pem = OpenSSLWrapper::publicKeyToPEM(pub);
  EVP_PKEY_free(pub);
  return pem;
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

  std::string addressPem = getAddressPem();
  if (txn.from.empty()) {
    txn.from = addressPem;
  } else if (txn.from != addressPem) {
    throw std::runtime_error(
        "Error: txn sender does not match wallet identity");
  }

  auto signableJson = txn.toSignableJson();
  auto sigBytes = sign(signableJson.dump());
  txn.signature = HexUtils::toHex(sigBytes);
}