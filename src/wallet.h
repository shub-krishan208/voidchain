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
  EVP_PKEY *getAddress() const;
  std::vector<unsigned char> sign(const std::string &data) const;
  void signTxn(Txn &txn) const;

private:
  EVP_PKEY *keyPair_;
};