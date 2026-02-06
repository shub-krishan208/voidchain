#include "wallet.h"

Wallet::Wallet() {
    keyPair_ = OpenSSLWrapper::generateKeyPair();
}

Wallet::~Wallet() {
    EVP_PKEY_free(keyPair_);
}

std::string Wallet::getAddress() const {
    return OpenSSLWrapper::publicKeyToHex(keyPair_);
}

EVP_PKEY* Wallet::getPublicKey() const {
    return keyPair_;
}

std::vector<unsigned char> Wallet::sign(const std::string& data) const {
    return OpenSSLWrapper::sign(keyPair_, data);
}