#include "hashing.h"
#include <sstream>

#include <iomanip>

Hasher::Hasher(){
    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
}

Hasher::~Hasher(){
    EVP_MD_CTX_free(ctx);
}

void Hasher::add(const void* data, size_t size){
    EVP_DigestUpdate(ctx, data, size);
}

// simpler version to only add a string
void Hasher::add(const std::string& s){
    add(s.data(), s.size());
}

std::string Hasher::finish(){
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int length = 0;

    EVP_DigestFinal_ex(ctx, hash, &length);

    std::ostringstream oss;
    for (unsigned i = 0; i < length; i++)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}