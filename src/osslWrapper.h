#pragma once

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include <string>
#include <vector>

class OpenSSLWrapper {
public:
  static EVP_PKEY *generateKeyPair();
  static std::vector<unsigned char> sign(EVP_PKEY *privateKey,
                                         const std::string &data);
  static bool verify(EVP_PKEY *publicKey, const std::string &data,
                     const std::vector<unsigned char> &signature);

  static EVP_PKEY *extractPublicKey(const EVP_PKEY *keypair);
  static std::string publicKeyToPEM(EVP_PKEY *publicKey);
};
