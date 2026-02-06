#pragma once

#include <string>
#include <vector>
#include "osslWrapper.h"

class Wallet {
public:
    Wallet();
    ~Wallet();

    EVP_PKEY* getPublicKey() const;
    EVP_PKEY* getAddress() const;
    std::vector<unsigned char> sign(const std::string& data) const;
private:
    EVP_PKEY* keyPair_;
};