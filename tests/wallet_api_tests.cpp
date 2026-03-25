#include "../src/TxnPool.h"
#include "../src/chain.h"
#include "../src/core/State.h"
#include "../src/miner.h"
#include "../src/models/AssetTxn.h"
#include "../src/models/CurrencyTxn.h"
#include "../src/osslWrapper.h"
#include "../src/utils/hex.h"
#include "../src/utils/uuid.h"
#include "../src/wallet.h"

#include <gtest/gtest.h>
#include <cctype>

namespace {
std::string addressFromKeyPair(EVP_PKEY *keyPair) {
  EVP_PKEY *pub = OpenSSLWrapper::extractPublicKey(keyPair);
  if (!pub) {
    return {};
  }

  std::string address = OpenSSLWrapper::publicKeyToAddress(pub);
  EVP_PKEY_free(pub);
  return address;
}

void signTxnWithKey(Txn &txn, EVP_PKEY *keyPair) {
  EVP_PKEY *pub = OpenSSLWrapper::extractPublicKey(keyPair);
  if (!pub) {
    txn.signature.clear();
    return;
  }
  txn.senderPubKey = OpenSSLWrapper::publicKeyToRawHex(pub);
  EVP_PKEY_free(pub);

  auto sigBytes = OpenSSLWrapper::sign(keyPair, txn.toSignableJson().dump());
  txn.signature = HexUtils::toHex(sigBytes);
}

std::string toUppercase(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  return value;
}

std::string withoutPrefix(const std::string &value) {
  if (value.rfind("0x", 0) == 0 || value.rfind("0X", 0) == 0) {
    return value.substr(2);
  }
  return value;
}
} // namespace

TEST(WalletApiTest, PrivateKeyHexRoundTripRestoresSameAddress) {
  EVP_PKEY *generated = OpenSSLWrapper::generateKeyPair();
  ASSERT_NE(generated, nullptr);

  const std::string originalAddress = addressFromKeyPair(generated);
  const std::string privateHex = OpenSSLWrapper::privateKeyToHex(generated);
  ASSERT_EQ(privateHex.size(), 64u);

  EVP_PKEY *restored = OpenSSLWrapper::keyPairFromPrivateHex(privateHex);
  ASSERT_NE(restored, nullptr);
  const std::string restoredAddress = addressFromKeyPair(restored);

  EXPECT_EQ(originalAddress, restoredAddress);

  EVP_PKEY_free(restored);
  EVP_PKEY_free(generated);
}

TEST(WalletApiTest, SignedCurrencyTransactionFlowUpdatesBalances) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet minerWallet;
  Miner miner(blockchain, pool, minerWallet);

  EVP_PKEY *senderKey = OpenSSLWrapper::generateKeyPair();
  EVP_PKEY *recipientKey = OpenSSLWrapper::generateKeyPair();
  ASSERT_NE(senderKey, nullptr);
  ASSERT_NE(recipientKey, nullptr);

  const std::string senderAddress = addressFromKeyPair(senderKey);
  const std::string recipientAddress = addressFromKeyPair(recipientKey);

  // Fund sender first via mine-to-address.
  miner.mine(senderAddress);

  auto spendTxn = std::make_shared<CurrencyTxn>();
  spendTxn->id = generateUUID();
  spendTxn->from = senderAddress;
  spendTxn->to = recipientAddress;
  spendTxn->amount = 10.0;
  signTxnWithKey(*spendTxn, senderKey);

  auto admission =
      State::validatePoolAdmission(spendTxn, blockchain.getChain(), pool.getTxn());
  ASSERT_TRUE(admission.ok);
  ASSERT_TRUE(pool.addTxn(spendTxn));

  miner.mine();

  DerivedState state = State::deriveFromChainOrThrow(blockchain.getChain());
  EXPECT_NEAR(State::getBalance(state, senderAddress), 40.0, 1e-9);
  EXPECT_NEAR(State::getBalance(state, recipientAddress), 10.0, 1e-9);

  EVP_PKEY_free(recipientKey);
  EVP_PKEY_free(senderKey);
}

TEST(WalletApiTest, SignedAssetMintFlowEstablishesOwnership) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet minerWallet;
  Miner miner(blockchain, pool, minerWallet);

  EVP_PKEY *ownerKey = OpenSSLWrapper::generateKeyPair();
  ASSERT_NE(ownerKey, nullptr);
  const std::string ownerAddress = addressFromKeyPair(ownerKey);

  // Fund owner address first to mirror intended API flow.
  miner.mine(ownerAddress);

  auto mintTxn = std::make_shared<AssetTxn>();
  mintTxn->id = generateUUID();
  mintTxn->from = ownerAddress;
  mintTxn->to = ownerAddress;
  mintTxn->itemId = "asset-wallet-api-1";
  mintTxn->meta = "minted-from-signed-endpoint-flow";
  signTxnWithKey(*mintTxn, ownerKey);

  auto admission =
      State::validatePoolAdmission(mintTxn, blockchain.getChain(), pool.getTxn());
  ASSERT_TRUE(admission.ok);
  ASSERT_TRUE(pool.addTxn(mintTxn));

  miner.mine();

  DerivedState state = State::deriveFromChainOrThrow(blockchain.getChain());
  EXPECT_EQ(State::getOwner(state, "asset-wallet-api-1"), ownerAddress);

  EVP_PKEY_free(ownerKey);
}

TEST(WalletApiTest, InvalidSecretKeyIsRejected) {
  EXPECT_EQ(OpenSSLWrapper::keyPairFromPrivateHex(""), nullptr);
  EXPECT_EQ(OpenSSLWrapper::keyPairFromPrivateHex("not-a-hex-secret"), nullptr);
  EXPECT_EQ(OpenSSLWrapper::keyPairFromPrivateHex("ab"), nullptr);
}

TEST(WalletApiTest, MineToAddressUsesProvidedRecipient) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet minerWallet;
  Miner miner(blockchain, pool, minerWallet);

  EVP_PKEY *beneficiaryKey = OpenSSLWrapper::generateKeyPair();
  ASSERT_NE(beneficiaryKey, nullptr);
  const std::string beneficiaryAddress = addressFromKeyPair(beneficiaryKey);

  const Block mined = miner.mine(beneficiaryAddress);
  const auto &txns = mined.getTransactions();
  ASSERT_FALSE(txns.empty());

  auto rewardTxn = std::dynamic_pointer_cast<CurrencyTxn>(txns.front());
  ASSERT_NE(rewardTxn, nullptr);
  EXPECT_EQ(rewardTxn->from, "COINBASE");
  EXPECT_EQ(rewardTxn->to, beneficiaryAddress);
  EXPECT_DOUBLE_EQ(rewardTxn->amount, Miner::MINING_REWARD);

  EVP_PKEY_free(beneficiaryKey);
}

TEST(WalletApiTest, UppercaseRecipientAddressStillCreditsCanonicalWallet) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet minerWallet;
  Miner miner(blockchain, pool, minerWallet);

  EVP_PKEY *senderKey = OpenSSLWrapper::generateKeyPair();
  EVP_PKEY *recipientKey = OpenSSLWrapper::generateKeyPair();
  ASSERT_NE(senderKey, nullptr);
  ASSERT_NE(recipientKey, nullptr);

  const std::string senderAddress = addressFromKeyPair(senderKey);
  const std::string recipientAddress = addressFromKeyPair(recipientKey);

  // Fund sender first via mine-to-address.
  miner.mine(senderAddress);

  auto spendTxn = std::make_shared<CurrencyTxn>();
  spendTxn->id = generateUUID();
  spendTxn->from = senderAddress;
  spendTxn->to = toUppercase(recipientAddress);
  spendTxn->amount = 10.0;
  signTxnWithKey(*spendTxn, senderKey);

  auto admission =
      State::validatePoolAdmission(spendTxn, blockchain.getChain(), pool.getTxn());
  ASSERT_TRUE(admission.ok);
  ASSERT_TRUE(pool.addTxn(spendTxn));

  miner.mine();

  DerivedState state = State::deriveFromChainOrThrow(blockchain.getChain());
  EXPECT_NEAR(State::getBalance(state, senderAddress), 40.0, 1e-9);
  EXPECT_NEAR(State::getBalance(state, recipientAddress), 10.0, 1e-9);
  EXPECT_NEAR(State::getBalance(state, spendTxn->to), 10.0, 1e-9);

  EVP_PKEY_free(recipientKey);
  EVP_PKEY_free(senderKey);
}

TEST(WalletApiTest, PrefixlessRecipientAddressStillCreditsCanonicalWallet) {
  Blockchain blockchain;
  TxnPool pool;
  Wallet minerWallet;
  Miner miner(blockchain, pool, minerWallet);

  EVP_PKEY *senderKey = OpenSSLWrapper::generateKeyPair();
  EVP_PKEY *recipientKey = OpenSSLWrapper::generateKeyPair();
  ASSERT_NE(senderKey, nullptr);
  ASSERT_NE(recipientKey, nullptr);

  const std::string senderAddress = addressFromKeyPair(senderKey);
  const std::string recipientAddress = addressFromKeyPair(recipientKey);

  // Fund sender first via mine-to-address.
  miner.mine(senderAddress);

  auto spendTxn = std::make_shared<CurrencyTxn>();
  spendTxn->id = generateUUID();
  spendTxn->from = senderAddress;
  spendTxn->to = withoutPrefix(recipientAddress);
  spendTxn->amount = 11.0;
  signTxnWithKey(*spendTxn, senderKey);

  auto admission =
      State::validatePoolAdmission(spendTxn, blockchain.getChain(), pool.getTxn());
  ASSERT_TRUE(admission.ok);
  ASSERT_TRUE(pool.addTxn(spendTxn));

  miner.mine();

  DerivedState state = State::deriveFromChainOrThrow(blockchain.getChain());
  EXPECT_NEAR(State::getBalance(state, senderAddress), 39.0, 1e-9);
  EXPECT_NEAR(State::getBalance(state, recipientAddress), 11.0, 1e-9);
  EXPECT_NEAR(State::getBalance(state, spendTxn->to), 11.0, 1e-9);

  EVP_PKEY_free(recipientKey);
  EVP_PKEY_free(senderKey);
}
