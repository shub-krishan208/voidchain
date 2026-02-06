#include "wallet.h"

Wallet::Wallet() {
    keyPair_ = OpenSSLWrapper::generateKeyPair();
}

Wallet::~Wallet() {
    EVP_PKEY_free(keyPair_);
}

EVP_PKEY* Wallet::getAddress() const {
    return keyPair_;
}

EVP_PKEY* Wallet::getPublicKey() const {
    return OpenSSLWrapper::extractPublicKey(keyPair_);
}

std::vector<unsigned char> Wallet::sign(const std::string& data) const {
    return OpenSSLWrapper::sign(keyPair_, data);
}