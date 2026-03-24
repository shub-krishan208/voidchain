#include "osslWrapper.h"
#include "utils/hex.h"

#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/param_build.h>

#include <cctype>
#include <iostream>
#include <vector>

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
  if (EVP_DigestSignFinal(mdctx, nullptr, &len) != 1) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }

  if (len == 0) {
    EVP_MD_CTX_free(mdctx);
    return {};
  }

  std::vector<unsigned char> signature(len);

  if (EVP_DigestSignFinal(mdctx, signature.data(), &len) != 1) {
    EVP_MD_CTX_free(mdctx);
    std::cerr << "Finalizing signature failed.\n";
    return {};
  }

  signature.resize(len);
  EVP_MD_CTX_free(mdctx);
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

  const int res =
      EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());

  EVP_MD_CTX_free(mdctx);
  return res == 1;
}

std::string OpenSSLWrapper::publicKeyToPEM(EVP_PKEY *pub) {
  BIO *bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PUBKEY(bio, pub);

  char *data;
  long len = BIO_get_mem_data(bio, &data);

  std::string pem(data, len);
  BIO_free(bio);

  while (!pem.empty() &&
         std::isspace(static_cast<unsigned char>(pem.back())) != 0) {
    pem.pop_back();
  }

  return pem;
}

EVP_PKEY *OpenSSLWrapper::extractPublicKey(const EVP_PKEY *keypair) {
  if (!keypair)
    return nullptr;

  BIO *bio = BIO_new(BIO_s_mem());
  if (!bio)
    return nullptr;

  if (i2d_PUBKEY_bio(bio, const_cast<EVP_PKEY *>(keypair)) != 1) {
    BIO_free(bio);
    return nullptr;
  }

  EVP_PKEY *pub = d2i_PUBKEY_bio(bio, nullptr);

  BIO_free(bio);
  return pub;
}

std::string OpenSSLWrapper::privateKeyToHex(EVP_PKEY *keyPair) {
  if (!keyPair) {
    return {};
  }

  BIGNUM *priv = nullptr;
  if (EVP_PKEY_get_bn_param(keyPair, OSSL_PKEY_PARAM_PRIV_KEY, &priv) != 1 ||
      !priv) {
    return {};
  }

  std::vector<unsigned char> bytes(32, 0);
  if (BN_bn2binpad(priv, bytes.data(), static_cast<int>(bytes.size())) !=
      static_cast<int>(bytes.size())) {
    BN_clear_free(priv);
    return {};
  }

  BN_clear_free(priv);
  const std::string hex = HexUtils::toHex(bytes);
  OPENSSL_cleanse(bytes.data(), bytes.size());
  return hex;
}

EVP_PKEY *OpenSSLWrapper::keyPairFromPrivateHex(const std::string &privateHex) {
  if (privateHex.size() != 64) {
    return nullptr;
  }

  std::vector<unsigned char> privBytes;
  try {
    privBytes = HexUtils::fromHex(privateHex);
  } catch (const std::exception &) {
    return nullptr;
  }

  if (privBytes.size() != 32) {
    return nullptr;
  }

  EVP_PKEY *keyPair = nullptr;
  BIGNUM *priv = BN_bin2bn(privBytes.data(), static_cast<int>(privBytes.size()),
                           nullptr);
  BN_CTX *bnCtx = BN_CTX_new();
  EC_GROUP *group = EC_GROUP_new_by_curve_name(NID_secp256k1);
  BIGNUM *order = BN_new();
  EC_POINT *pubPoint = nullptr;
  OSSL_PARAM_BLD *paramBld = nullptr;
  OSSL_PARAM *params = nullptr;
  EVP_PKEY_CTX *pkeyCtx = nullptr;

  size_t pubLen = 0;
  std::vector<unsigned char> pubBytes;
  bool ok = false;

  do {
    if (!priv || !bnCtx || !group || !order) {
      break;
    }

    if (EC_GROUP_get_order(group, order, bnCtx) != 1) {
      break;
    }

    if (BN_is_zero(priv) || BN_is_negative(priv) || BN_cmp(priv, order) >= 0) {
      break;
    }

    pubPoint = EC_POINT_new(group);
    if (!pubPoint) {
      break;
    }

    if (EC_POINT_mul(group, pubPoint, priv, nullptr, nullptr, bnCtx) != 1) {
      break;
    }

    pubLen = EC_POINT_point2oct(group, pubPoint, POINT_CONVERSION_UNCOMPRESSED,
                                nullptr, 0, bnCtx);
    if (pubLen == 0) {
      break;
    }

    pubBytes.resize(pubLen);
    if (EC_POINT_point2oct(group, pubPoint, POINT_CONVERSION_UNCOMPRESSED,
                           pubBytes.data(), pubBytes.size(),
                           bnCtx) != pubBytes.size()) {
      break;
    }

    paramBld = OSSL_PARAM_BLD_new();
    if (!paramBld) {
      break;
    }

    if (OSSL_PARAM_BLD_push_utf8_string(paramBld, OSSL_PKEY_PARAM_GROUP_NAME,
                                        "secp256k1", 0) != 1) {
      break;
    }

    if (OSSL_PARAM_BLD_push_BN(paramBld, OSSL_PKEY_PARAM_PRIV_KEY, priv) != 1) {
      break;
    }

    if (OSSL_PARAM_BLD_push_octet_string(paramBld, OSSL_PKEY_PARAM_PUB_KEY,
                                         pubBytes.data(), pubBytes.size()) != 1) {
      break;
    }

    params = OSSL_PARAM_BLD_to_param(paramBld);
    if (!params) {
      break;
    }

    pkeyCtx = EVP_PKEY_CTX_new_from_name(nullptr, "EC", nullptr);
    if (!pkeyCtx) {
      break;
    }

    if (EVP_PKEY_fromdata_init(pkeyCtx) <= 0) {
      break;
    }

    if (EVP_PKEY_fromdata(pkeyCtx, &keyPair, EVP_PKEY_KEYPAIR, params) <= 0) {
      break;
    }

    ok = true;
  } while (false);

  if (!ok) {
    EVP_PKEY_free(keyPair);
    keyPair = nullptr;
  }

  if (!privBytes.empty()) {
    OPENSSL_cleanse(privBytes.data(), privBytes.size());
  }
  if (!pubBytes.empty()) {
    OPENSSL_cleanse(pubBytes.data(), pubBytes.size());
  }
  EVP_PKEY_CTX_free(pkeyCtx);
  OSSL_PARAM_free(params);
  OSSL_PARAM_BLD_free(paramBld);
  EC_POINT_free(pubPoint);
  BN_free(order);
  EC_GROUP_free(group);
  BN_CTX_free(bnCtx);
  BN_clear_free(priv);
  return keyPair;
}
