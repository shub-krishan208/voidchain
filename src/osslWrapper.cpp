#include "osslWrapper.h"
#include <iostream>

EVP_PKEY *OpenSSLWrapper::generateKeyPair() {
  EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
  if (!ctx)
    return nullptr;

  EVP_PKEY *pkey = nullptr;

  if (EVP_PKEY_keygen_init(ctx) <= 0)
    goto err;
  if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_secp256k1) <= 0)
    goto err;
  if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    goto err;

  EVP_PKEY_CTX_free(ctx);
  return pkey;

err:
  EVP_PKEY_CTX_free(ctx);
  return nullptr;
}

std::vector<unsigned char> OpenSSLWrapper::sign(EVP_PKEY *privateKey,
                                                const std::string &data) {
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (!mdctx)
    return {};

  if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, privateKey) !=
      1) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }

  if (EVP_DigestSignUpdate(mdctx, data.data(), data.size()) != 1) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }

  size_t len = 0;
  // Get the required size
  if (EVP_DigestSignFinal(mdctx, nullptr, &len) != 1) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }

  if (len == 0) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }

  std::vector<unsigned char> signature(len);

  // CRITICAL: Check the return value of the actual signing
  if (EVP_DigestSignFinal(mdctx, signature.data(), &len) != 1) {
    EVP_MD_CTX_free(mdctx);
    std::cerr << "Finalizing signature failed.\n";
    return {};
  }

  // Resize vector to actual length written (in case it's smaller than max)
  signature.resize(len);

  EVP_MD_CTX_free(mdctx);

  // Debug print
  // for (unsigned char b : signature)
  // printf("%02x", b);
  // printf("\n");

  return signature;
}

bool OpenSSLWrapper::verify(EVP_PKEY *pkey, const std::string &data,
                            const std::vector<unsigned char> &signature) {
  if (!pkey || signature.empty()) {
    std::cerr << "Invalid public key or empty signature for verification.\n";
    return false;
  }

  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (!mdctx) {
    std::cerr << "Failed to create MD_CTX for verification.\n";
    return false;
  }

  if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey) != 1) {
    EVP_MD_CTX_free(mdctx);
    std::cerr << "EVP_DigestVerifyInit failed.\n";
    return false;
  }

  if (EVP_DigestVerifyUpdate(mdctx, data.data(), data.size()) != 1) {
    EVP_MD_CTX_free(mdctx);
    std::cerr << "EVP_DigestVerifyUpdate failed.\n";
    return false;
  }

  int res = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());

  EVP_MD_CTX_free(mdctx);
  return res == 1;
}

std::string OpenSSLWrapper::publicKeyToPEM(EVP_PKEY *pub) {
  BIO *bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PUBKEY(bio, pub);

  char *data;
  long len = BIO_get_mem_data(bio, &data);

  std::string hex(data, len);
  BIO_free(bio);
  return hex;
}

EVP_PKEY *OpenSSLWrapper::extractPublicKey(const EVP_PKEY *keypair) {
  if (!keypair)
    return nullptr;

  // 1. Create a memory BIO
  BIO *bio = BIO_new(BIO_s_mem());
  if (!bio)
    return nullptr;

  // 2. Write ONLY the public key to the BIO
  // This automatically handles the curve parameters and format
  if (i2d_PUBKEY_bio(bio, const_cast<EVP_PKEY *>(keypair)) != 1) {
    BIO_free(bio);
    return nullptr;
  }

  // 3. Read it back into a NEW EVP_PKEY structure
  EVP_PKEY *pub = d2i_PUBKEY_bio(bio, nullptr);

  BIO_free(bio);
  return pub;
}
