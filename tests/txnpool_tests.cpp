#include "../src/TxnPool.h"
#include "../src/models/AssetTxn.h"
#include "../src/models/CurrencyTxn.h"
#include "../src/osslWrapper.h"
#include "../src/utils/hex.h"
#include "../src/wallet.h"
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// Helper: create a properly-signed CurrencyTxn via a Wallet
// ---------------------------------------------------------------------------
static std::shared_ptr<CurrencyTxn>
makeSignedCurrencyTxn(Wallet &w, const std::string &to, double amount) {
  auto txn = std::make_shared<CurrencyTxn>();
  txn->to = to;
  txn->amount = amount;

  // signTxn requires a Txn&, so dereference and then copy back via shared_ptr
  w.signTxn(*txn);

  // after signing, `from` is still whatever we left it — the pool's verifyTxn
  // needs `from` to hold the PEM public key so the signature can be checked.
  EVP_PKEY *pub = w.getPublicKey();
  txn->from = OpenSSLWrapper::publicKeyToPEM(pub);
  EVP_PKEY_free(pub);

  // re-sign because `from` changed after the first sign
  // we need to clear signature first and resign with correct `from`
  txn->signature.clear();
  txn->id.clear();
  w.signTxn(*txn);

  return txn;
}

// Helper: create a properly-signed AssetTxn
static std::shared_ptr<AssetTxn> makeSignedAssetTxn(Wallet &w,
                                                    const std::string &to,
                                                    const std::string &itemId,
                                                    const std::string &meta) {
  auto txn = std::make_shared<AssetTxn>();
  EVP_PKEY *pub = w.getPublicKey();
  txn->from = OpenSSLWrapper::publicKeyToPEM(pub);
  EVP_PKEY_free(pub);
  txn->to = to;
  txn->itemId = itemId;
  txn->meta = meta;
  w.signTxn(*txn);
  return txn;
}

// ===========================================================================
//  addTxn — happy paths
// ===========================================================================

TEST(TxnPoolTest, AddValidCurrencyTxn) {
  TxnPool pool;
  Wallet w;
  auto txn = makeSignedCurrencyTxn(w, "bob", 10.0);

  EXPECT_TRUE(pool.addTxn(txn));

  auto all = pool.getTxn();
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0]->id, txn->id);
}

TEST(TxnPoolTest, AddValidAssetTxn) {
  TxnPool pool;
  Wallet w;
  auto txn = makeSignedAssetTxn(w, "bob", "item-42", "legendary sword");

  EXPECT_TRUE(pool.addTxn(txn));

  auto all = pool.getTxn();
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0]->id, txn->id);
}

TEST(TxnPoolTest, AddMultipleTransactions) {
  TxnPool pool;
  Wallet w;

  auto t1 = makeSignedCurrencyTxn(w, "bob", 1.0);
  auto t2 = makeSignedCurrencyTxn(w, "charlie", 2.0);
  auto t3 = makeSignedAssetTxn(w, "dave", "nft-01", "rare art");

  EXPECT_TRUE(pool.addTxn(t1));
  EXPECT_TRUE(pool.addTxn(t2));
  EXPECT_TRUE(pool.addTxn(t3));

  EXPECT_EQ(pool.getTxn().size(), 3u);
}

TEST(TxnPoolTest, AddTxnFromDifferentWallets) {
  TxnPool pool;
  Wallet w1, w2;

  auto t1 = makeSignedCurrencyTxn(w1, "bob", 5.0);
  auto t2 = makeSignedCurrencyTxn(w2, "bob", 5.0);

  EXPECT_TRUE(pool.addTxn(t1));
  EXPECT_TRUE(pool.addTxn(t2));
  EXPECT_EQ(pool.getTxn().size(), 2u);
}

// ===========================================================================
//  addTxn — rejection / failure paths (verifyTxn)
// ===========================================================================

TEST(TxnPoolTest, RejectNullTxn) {
  TxnPool pool;
  EXPECT_FALSE(pool.addTxn(nullptr));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RejectTxnWithEmptySignature) {
  TxnPool pool;
  Wallet w;

  auto txn = std::make_shared<CurrencyTxn>();
  EVP_PKEY *pub = w.getPublicKey();
  txn->from = OpenSSLWrapper::publicKeyToPEM(pub);
  EVP_PKEY_free(pub);
  txn->to = "bob";
  txn->amount = 10.0;
  txn->id = "test-id";
  // signature left empty

  EXPECT_FALSE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RejectTxnWithEmptyFrom) {
  TxnPool pool;
  Wallet w;

  auto txn = std::make_shared<CurrencyTxn>();
  txn->from = ""; // empty sender
  txn->to = "bob";
  txn->amount = 10.0;
  txn->id = "test-id";
  txn->signature = "deadbeef";

  EXPECT_FALSE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RejectTxnWithInvalidPEMKey) {
  TxnPool pool;

  auto txn = std::make_shared<CurrencyTxn>();
  txn->from = "not-a-valid-pem-key";
  txn->to = "bob";
  txn->amount = 10.0;
  txn->id = "test-id";
  txn->signature = "deadbeef";

  EXPECT_FALSE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RejectTxnWithWrongSignature) {
  TxnPool pool;
  Wallet w;

  // create a valid txn but tamper with the signature
  auto txn = makeSignedCurrencyTxn(w, "bob", 50.0);
  txn->signature = "badc0ffee000"; // garbage hex
  EXPECT_FALSE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RejectTxnSignedByDifferentWallet) {
  TxnPool pool;
  Wallet signer, impersonator;

  // sign with `signer` wallet but use `impersonator`'s public key as `from`
  auto txn = std::make_shared<CurrencyTxn>();
  EVP_PKEY *impPub = impersonator.getPublicKey();
  txn->from = OpenSSLWrapper::publicKeyToPEM(impPub);
  EVP_PKEY_free(impPub);
  txn->to = "bob";
  txn->amount = 100.0;

  // sign with the wrong wallet
  signer.signTxn(*txn);

  EXPECT_FALSE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RejectTxnWithTamperedData) {
  TxnPool pool;
  Wallet w;

  auto txn = makeSignedCurrencyTxn(w, "bob", 10.0);
  // tamper after signing: change the amount
  txn->amount = 99999.0;

  EXPECT_FALSE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RejectAssetTxnWithTamperedItemId) {
  TxnPool pool;
  Wallet w;

  auto txn = makeSignedAssetTxn(w, "bob", "item-01", "diamond");
  // tamper: change itemId after signing
  txn->itemId = "item-999";

  EXPECT_FALSE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

// ===========================================================================
//  addTxn — duplicate handling
// ===========================================================================

TEST(TxnPoolTest, DuplicateIdOverwritesPreviousTxn) {
  TxnPool pool;
  Wallet w;

  auto t1 = makeSignedCurrencyTxn(w, "bob", 1.0);
  EXPECT_TRUE(pool.addTxn(t1));

  // add a different txn but give it the same id
  auto t2 = makeSignedCurrencyTxn(w, "charlie", 2.0);
  t2->id = t1->id;
  // resign because id changed
  t2->signature.clear();
  w.signTxn(*t2);

  EXPECT_FALSE(pool.addTxn(t2));
  // map overwrites: size should still be 1
  EXPECT_EQ(pool.getTxn().size(), 1u);
}

// ===========================================================================
//  getTxn
// ===========================================================================

TEST(TxnPoolTest, GetTxnReturnsEmptyForEmptyPool) {
  TxnPool pool;
  auto all = pool.getTxn();
  EXPECT_TRUE(all.empty());
}

TEST(TxnPoolTest, GetTxnReturnsAllAdded) {
  TxnPool pool;
  Wallet w;

  auto t1 = makeSignedCurrencyTxn(w, "a", 1.0);
  auto t2 = makeSignedCurrencyTxn(w, "b", 2.0);

  pool.addTxn(t1);
  pool.addTxn(t2);

  auto all = pool.getTxn();
  ASSERT_EQ(all.size(), 2u);

  // collect ids
  std::set<std::string> ids;
  for (auto &t : all)
    ids.insert(t->id);
  EXPECT_TRUE(ids.count(t1->id));
  EXPECT_TRUE(ids.count(t2->id));
}

// ===========================================================================
//  remove
// ===========================================================================

TEST(TxnPoolTest, RemoveExistingTxn) {
  TxnPool pool;
  Wallet w;

  auto t1 = makeSignedCurrencyTxn(w, "bob", 1.0);
  auto t2 = makeSignedCurrencyTxn(w, "charlie", 2.0);
  pool.addTxn(t1);
  pool.addTxn(t2);

  pool.remove(t1->id);

  auto all = pool.getTxn();
  ASSERT_EQ(all.size(), 1u);
  EXPECT_EQ(all[0]->id, t2->id);
}

TEST(TxnPoolTest, RemoveNonExistentIdDoesNothing) {
  TxnPool pool;
  Wallet w;

  auto txn = makeSignedCurrencyTxn(w, "bob", 1.0);
  pool.addTxn(txn);

  pool.remove("non-existent-id");

  EXPECT_EQ(pool.getTxn().size(), 1u);
}

TEST(TxnPoolTest, RemoveFromEmptyPoolDoesNothing) {
  TxnPool pool;
  EXPECT_NO_THROW(pool.remove("any-id"));
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, RemoveAllTxnsOneByOne) {
  TxnPool pool;
  Wallet w;

  auto t1 = makeSignedCurrencyTxn(w, "a", 1.0);
  auto t2 = makeSignedCurrencyTxn(w, "b", 2.0);
  auto t3 = makeSignedCurrencyTxn(w, "c", 3.0);
  pool.addTxn(t1);
  pool.addTxn(t2);
  pool.addTxn(t3);

  pool.remove(t1->id);
  EXPECT_EQ(pool.getTxn().size(), 2u);

  pool.remove(t2->id);
  EXPECT_EQ(pool.getTxn().size(), 1u);

  pool.remove(t3->id);
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

// ===========================================================================
//  clear
// ===========================================================================

TEST(TxnPoolTest, ClearEmptiesThePool) {
  TxnPool pool;
  Wallet w;

  pool.addTxn(makeSignedCurrencyTxn(w, "a", 1.0));
  pool.addTxn(makeSignedCurrencyTxn(w, "b", 2.0));
  pool.addTxn(makeSignedCurrencyTxn(w, "c", 3.0));

  EXPECT_EQ(pool.getTxn().size(), 3u);

  pool.clear();
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, ClearOnEmptyPoolDoesNothing) {
  TxnPool pool;
  EXPECT_NO_THROW(pool.clear());
  EXPECT_EQ(pool.getTxn().size(), 0u);
}

TEST(TxnPoolTest, ClearThenAddNewTxns) {
  TxnPool pool;
  Wallet w;

  pool.addTxn(makeSignedCurrencyTxn(w, "a", 1.0));
  pool.clear();
  EXPECT_EQ(pool.getTxn().size(), 0u);

  // pool should still be functional after clear
  auto txn = makeSignedCurrencyTxn(w, "b", 2.0);
  EXPECT_TRUE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 1u);
}

// ===========================================================================
//  Combined operations / edge cases
// ===========================================================================

TEST(TxnPoolTest, RemoveDoesNotAffectOtherTxns) {
  TxnPool pool;
  Wallet w;

  auto t1 = makeSignedCurrencyTxn(w, "a", 1.0);
  auto t2 = makeSignedCurrencyTxn(w, "b", 2.0);
  pool.addTxn(t1);
  pool.addTxn(t2);

  pool.remove(t1->id);
  auto remaining = pool.getTxn();
  ASSERT_EQ(remaining.size(), 1u);
  EXPECT_EQ(remaining[0]->id, t2->id);
  EXPECT_EQ(remaining[0]->getType(), "CURRENCY");
}

TEST(TxnPoolTest, PoolAcceptsBothTxnTypes) {
  TxnPool pool;
  Wallet w;

  auto currency = makeSignedCurrencyTxn(w, "bob", 5.0);
  auto asset = makeSignedAssetTxn(w, "bob", "nft-1", "meta");

  EXPECT_TRUE(pool.addTxn(currency));
  EXPECT_TRUE(pool.addTxn(asset));
  EXPECT_EQ(pool.getTxn().size(), 2u);
}

TEST(TxnPoolTest, AddAfterRemoveSameId) {
  TxnPool pool;
  Wallet w;

  auto txn = makeSignedCurrencyTxn(w, "bob", 10.0);
  std::string id = txn->id;

  pool.addTxn(txn);
  pool.remove(id);
  EXPECT_EQ(pool.getTxn().size(), 0u);

  // re-add with same shared_ptr
  EXPECT_TRUE(pool.addTxn(txn));
  EXPECT_EQ(pool.getTxn().size(), 1u);
}

TEST(TxnPoolTest, RejectedTxnDoesNotPollutePool) {
  TxnPool pool;
  Wallet w;

  // add one valid txn
  auto valid = makeSignedCurrencyTxn(w, "bob", 1.0);
  EXPECT_TRUE(pool.addTxn(valid));

  // attempt to add several invalid txns
  EXPECT_FALSE(pool.addTxn(nullptr));

  auto noSig = std::make_shared<CurrencyTxn>();
  noSig->from = "x";
  noSig->to = "y";
  noSig->amount = 1.0;
  EXPECT_FALSE(pool.addTxn(noSig));

  auto noFrom = std::make_shared<CurrencyTxn>();
  noFrom->signature = "aa";
  noFrom->to = "y";
  noFrom->amount = 1.0;
  EXPECT_FALSE(pool.addTxn(noFrom));

  // pool should still contain only the one valid txn
  EXPECT_EQ(pool.getTxn().size(), 1u);
  EXPECT_EQ(pool.getTxn()[0]->id, valid->id);
}
