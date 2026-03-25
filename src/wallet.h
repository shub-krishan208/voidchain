#pragma once

#include "models/Txn.h"
#include "osslWrapper.h"
#include <string>
#include <vector>

class Wallet {
public:
  Wallet();
  ~Wallet();

  EVP_PKEY *getPublicKey() const;
  std::string getAddress() const;
  std::string getPublicKeyHex() const;
  std::vector<unsigned char> sign(const std::string &data) const;
  void signTxn(Txn &txn) const;

private:
  EVP_PKEY *keyPair_;
};