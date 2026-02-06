#pragma once

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/pem.h>

#include <string>
#include <vector>

class OpenSSLWrapper {
public:
    static EVP_PKEY* generateKeyPair();
    static std::vector<unsigned char> sign(EVP_PKEY* privateKey, const std::string& data);
    static bool verify(EVP_PKEY* publicKey, const std::string& data, const std::vector<unsigned char>& signature);
    
    static std::string publicKeyToHex(EVP_PKEY* publicKey);
};