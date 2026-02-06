#pragma once

#include <string>
#include <vector>
#include "osslWrapper.h"

class Wallet {
public:
    Wallet();
    ~Wallet();

    EVP_PKEY* getPublicKey() const;
    std::vector<unsigned char> sign(const std::string& data) const;
    std::string getAddress() const;
private:
    EVP_PKEY* keyPair_;
};