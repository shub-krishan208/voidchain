#include "../src/wallet.h"
#include <gtest/gtest.h>
#include <openssl/evp.h>

TEST(WalletTest, WalletBasics) {
  Wallet w;

  EVP_PKEY *pub = w.getPublicKey();
  EXPECT_NE(pub, nullptr);

  auto address = w.getAddress();
  EXPECT_NE(address, nullptr);

  std::string data = "Test data for signing";
  std::vector<unsigned char> signature = w.sign(data);
  ASSERT_FALSE(signature.empty());

  ASSERT_NE(pub, nullptr);

  bool isValid = OpenSSLWrapper::verify(pub, data, signature);
  EXPECT_TRUE(isValid);

  EVP_PKEY_free(pub);
}
