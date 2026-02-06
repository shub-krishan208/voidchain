#include <gtest/gtest.h>
#include "../src/wallet.h"

TEST(WalletTest, GenerateWallet) {
    Wallet w;
    EVP_PKEY* pub = w.getPublicKey();
    EXPECT_NE(pub, nullptr);
}

TEST(WalletTest, GetAddress) {
    Wallet w;
    std::string address = w.getAddress();
    EXPECT_FALSE(address.empty());
}

TEST(WalletTest, SignAndVerify) {
    Wallet w;
    std::string data = "Test data for signing";
    std::vector<unsigned char> signature = w.sign(data);

    bool isValid = OpenSSLWrapper::verify(w.getPublicKey(), data, signature);
    EXPECT_TRUE(isValid);
}