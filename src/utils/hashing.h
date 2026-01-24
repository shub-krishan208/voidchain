#ifndef HASHING_H
#define HASHING_H

#include <cstddef>
#include <string>
#include <openssl/evp.h>

class Hasher {
public:
  Hasher();
  ~Hasher();

  // disable copy constructor and assignment operator
  Hasher(const Hasher &) = delete;
  Hasher &operator=(const Hasher &) = delete;
  // add new data to the evp digest
  void add(const void *data, size_t size);
  void add(const std::string& s);
  std::string finish(); // each hasher instance can only give one hash

private:
  EVP_MD_CTX* ctx;
};
#endif // HASHING_H