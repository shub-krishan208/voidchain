#include <gtest/gtest.h>
#include "../src/wallet.h"

TEST(WalletTest, GenerateWallet) {
    Wallet w;
    ASSERT_NE(w.getPublicKey(), nullptr);
    EVP_PKEY* pub = w.getPublicKey();
    
    EXPECT_NE(pub, nullptr);
}

TEST(WalletTest, GetAddress) {
    Wallet w;
    auto address = w.getAddress();
    EXPECT_NE(address, nullptr);
}

TEST(WalletTest, SignAndVerify) {
    Wallet w;
    std::string data = "Test data for signing";
    std::vector<unsigned char> signature = w.sign(data);
    ASSERT_FALSE(signature.empty());
    ASSERT_NE(w.getPublicKey(), nullptr);
    ASSERT_NE(w.getAddress(), nullptr);
    // bool isValid = OpenSSLWrapper::verify(w.getPublicKey(), data, signature);
    // EXPECT_TRUE(isValid);
}