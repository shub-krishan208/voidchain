#include "osslWrapper.h"

EVP_PKEY* OpenSSLWrapper::generateKeyPair() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    EVP_PKEY* pkey = nullptr;

    EVP_PKEY_keygen_init(ctx);
    EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_secp256k1);
    EVP_PKEY_keygen(ctx, &pkey);

    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

std::vector<unsigned char> OpenSSLWrapper::sign(
    EVP_PKEY* privateKey, 
    const std::string& data
) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

    EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, privateKey);

    EVP_DigestSignUpdate(mdctx, data.data(), data.size());

    size_t len =0;
    EVP_DigestSignFinal(mdctx, nullptr, &len);

    std::vector<unsigned char> signature(len);
    EVP_DigestSignFinal(mdctx, signature.data(), &len);

    EVP_MD_CTX_free(mdctx);
    return signature;
}

bool OpenSSLWrapper::verify(
    EVP_PKEY* pkey, 
    const std::string& data, 
    const std::vector<unsigned char>& signature
) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestVerifyUpdate(mdctx, data.data(), data.size());

    int res = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());

    EVP_MD_CTX_free(mdctx);
    return res == 1;
}

// get wallet address from pubkey
std::string OpenSSLWrapper::publicKeyToHex(EVP_PKEY* pub){
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, pub);

    char* data;
    long len = BIO_get_mem_data(bio, &data);

    std::string hex(data, len);
    BIO_free(bio);
    return hex;
}