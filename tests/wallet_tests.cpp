#include "../src/models/AssetTxn.h"
#include "../src/models/CurrencyTxn.h"
#include "../src/utils/hex.h"
#include "../src/wallet.h"
#include <gtest/gtest.h>
#include <openssl/evp.h>

TEST(WalletTest, WalletBasics) {
  Wallet w;

  EVP_PKEY *pub = w.getPublicKey();
  EXPECT_NE(pub, nullptr);

  auto address = w.getAddress();
  EXPECT_FALSE(address.empty());
  EXPECT_EQ(address.rfind("0x", 0), 0u);

  std::string data = "Test data for signing";
  std::vector<unsigned char> signature = w.sign(data);
  ASSERT_FALSE(signature.empty());

  ASSERT_NE(pub, nullptr);

  bool isValid = OpenSSLWrapper::verify(pub, data, signature);
  EXPECT_TRUE(isValid);

  EVP_PKEY_free(pub);
}

// ---- signTxn tests ----

TEST(WalletTest, SignCurrencyTxn) {
  Wallet w;
  CurrencyTxn txn;
  txn.to = "bob";
  txn.amount = 42.5;

  w.signTxn(txn);

  // id should be assigned (UUID = 36 chars)
  EXPECT_FALSE(txn.id.empty());
  EXPECT_EQ(txn.id.size(), 36u);

  // signature should be populated
  EXPECT_FALSE(txn.signature.empty());
  EXPECT_FALSE(txn.from.empty());
  EXPECT_EQ(txn.from, w.getAddress());
  EXPECT_EQ(txn.senderPubKey, w.getPublicKeyHex());

  // verify the signature against the signable JSON
  auto sigBytes = HexUtils::fromHex(txn.signature);
  EVP_PKEY *pub = w.getPublicKey();
  ASSERT_NE(pub, nullptr);
  bool valid =
      OpenSSLWrapper::verify(pub, txn.toSignableJson().dump(), sigBytes);
  EXPECT_TRUE(valid);
  EVP_PKEY_free(pub);
}

TEST(WalletTest, SignAssetTxn) {
  Wallet w;
  AssetTxn txn;
  txn.to = "bob";
  txn.itemId = "item-001";
  txn.meta = "rare sword";

  w.signTxn(txn);

  EXPECT_FALSE(txn.id.empty());
  EXPECT_EQ(txn.id.size(), 36u);
  EXPECT_FALSE(txn.signature.empty());
  EXPECT_FALSE(txn.from.empty());
  EXPECT_EQ(txn.from, w.getAddress());
  EXPECT_EQ(txn.senderPubKey, w.getPublicKeyHex());

  auto sigBytes = HexUtils::fromHex(txn.signature);
  EVP_PKEY *pub = w.getPublicKey();
  ASSERT_NE(pub, nullptr);
  bool valid =
      OpenSSLWrapper::verify(pub, txn.toSignableJson().dump(), sigBytes);
  EXPECT_TRUE(valid);
  EVP_PKEY_free(pub);
}

TEST(WalletTest, SignTxnRejectsAlreadySigned) {
  Wallet w;
  CurrencyTxn txn;
  txn.to = "bob";
  txn.amount = 10.0;

  w.signTxn(txn);

  // signing the same txn again should throw
  EXPECT_THROW(w.signTxn(txn), std::runtime_error);
}

TEST(WalletTest, DifferentWalletsProduceDifferentSignatures) {
  Wallet w1;
  Wallet w2;

  CurrencyTxn txn1;
  txn1.to = "bob";
  txn1.amount = 5.0;

  CurrencyTxn txn2;
  txn2.to = "bob";
  txn2.amount = 5.0;

  w1.signTxn(txn1);
  w2.signTxn(txn2);

  // different wallets -> different signatures
  EXPECT_NE(txn1.signature, txn2.signature);

  // each signature should only verify with its own wallet's key
  auto sig1 = HexUtils::fromHex(txn1.signature);
  auto sig2 = HexUtils::fromHex(txn2.signature);

  EVP_PKEY *pub1 = w1.getPublicKey();
  EVP_PKEY *pub2 = w2.getPublicKey();

  EXPECT_TRUE(OpenSSLWrapper::verify(pub1, txn1.toSignableJson().dump(), sig1));
  EXPECT_FALSE(
      OpenSSLWrapper::verify(pub2, txn1.toSignableJson().dump(), sig1));

  EXPECT_TRUE(OpenSSLWrapper::verify(pub2, txn2.toSignableJson().dump(), sig2));
  EXPECT_FALSE(
      OpenSSLWrapper::verify(pub1, txn2.toSignableJson().dump(), sig2));

  EVP_PKEY_free(pub1);
  EVP_PKEY_free(pub2);
}

TEST(WalletTest, SignTxnRejectsMismatchedSender) {
  Wallet signer;
  Wallet other;

  CurrencyTxn txn;
  txn.from = other.getAddress();
  txn.to = "bob";
  txn.amount = 12.0;

  EXPECT_THROW(signer.signTxn(txn), std::runtime_error);
}
