#include "osslWrapper.h"
#include <iostream>

EVP_PKEY* OpenSSLWrapper::generateKeyPair() {
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if(!ctx) return nullptr;

    EVP_PKEY* pkey = nullptr;

    if (EVP_PKEY_keygen_init(ctx) <= 0) goto err;
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_secp256k1) <= 0) goto err;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) goto err;

    EVP_PKEY_CTX_free(ctx);
    return pkey;

err:
    EVP_PKEY_CTX_free(ctx);
    return nullptr;
}

std::vector<unsigned char> OpenSSLWrapper::sign(
    EVP_PKEY* privateKey, 
    const std::string& data
) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if(!mdctx) return {};

    if(EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, privateKey) != 1) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }

    if(EVP_DigestSignUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }

    size_t len =0;
    EVP_DigestSignFinal(mdctx, nullptr, &len);

    if(len == 0) {
        EVP_MD_CTX_free(mdctx);
        return {};
    }
    std::vector<unsigned char> signature(len);
    EVP_DigestSignFinal(mdctx, signature.data(), &len);
    
    EVP_MD_CTX_free(mdctx);
    std::cout << "Data signed successfully. Signature length: " << len << " bytes.\n" << signature.data() << std::endl;
    return signature;
}

bool OpenSSLWrapper::verify(
    EVP_PKEY* pkey, 
    const std::string& data, 
    const std::vector<unsigned char>& signature
) {
    if(!pkey || signature.empty()) {
        std::cerr << "Invalid public key or empty signature for verification.\n";
        return false;
    }

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if(!mdctx) {
        std::cerr << "Failed to create MD_CTX for verification.\n";        
        return false;
    }

    if(EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "EVP_DigestVerifyInit failed.\n";
        return false;
    }

    if(EVP_DigestVerifyUpdate(mdctx, data.data(), data.size()) != 1) {
        EVP_MD_CTX_free(mdctx);
        std::cerr << "EVP_DigestVerifyUpdate failed.\n";
        return false;
    }

    int res = EVP_DigestVerifyFinal(mdctx, signature.data(), signature.size());

    EVP_MD_CTX_free(mdctx);
    return res == 1;
}

std::string OpenSSLWrapper::publicKeyToPEM(EVP_PKEY* pub){
    BIO* bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, pub);

    char* data;
    long len = BIO_get_mem_data(bio, &data);

    std::string hex(data, len);
    BIO_free(bio);
    return hex;
}

EVP_PKEY* OpenSSLWrapper::extractPublicKey(const EVP_PKEY* keypair){
    EVP_PKEY* pub = EVP_PKEY_new();

    EVP_PKEY_set1_EC_KEY(pub, const_cast<EC_KEY*>(EVP_PKEY_get0_EC_KEY(keypair)));
    return pub;
}
