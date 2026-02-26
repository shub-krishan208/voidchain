#include "../src/core/MerkleTree.h"
#include "../src/models/CurrencyTxn.h"
#include "../src/utils/hashing.h"
#include <gtest/gtest.h>
#include <memory>

// helper: hash a txn the same way the tree does internally
static std::string hashTxn(const std::shared_ptr<Txn> &tx) {
  Hasher h;
  h.add(tx->toJson().dump());
  return h.finish();
}

TEST(MerkleTest, RootChanges) {

  auto tx1 = std::make_shared<CurrencyTxn>();
  tx1->amount = 10;

  auto tx2 = std::make_shared<CurrencyTxn>();
  tx2->amount = 20;

  std::vector<std::shared_ptr<Txn>> txs = {tx1, tx2};

  MerkleTree tree1(txs);
  std::string root1 = tree1.getRoot();

  tx2->amount = 999; // mutate

  MerkleTree tree2(txs);
  std::string root2 = tree2.getRoot();

  EXPECT_NE(root1, root2);

  auto tx3 = std::make_shared<CurrencyTxn>();
  tx3->amount = 30;

  txs.push_back(tx3);

  MerkleTree tree3(txs);
  std::string root3 = tree3.getRoot();

  EXPECT_NE(root1, root3);
  EXPECT_NE(root2, root3);
}

// ---------- getProof – even number of leaves ----------

TEST(MerkleTest, GetProofEvenTree) {
  auto tx1 = std::make_shared<CurrencyTxn>(); tx1->amount = 10;
  auto tx2 = std::make_shared<CurrencyTxn>(); tx2->amount = 20;
  auto tx3 = std::make_shared<CurrencyTxn>(); tx3->amount = 30;
  auto tx4 = std::make_shared<CurrencyTxn>(); tx4->amount = 40;

  std::vector<std::shared_ptr<Txn>> txs = {tx1, tx2, tx3, tx4};
  MerkleTree tree(txs);

  // 4 leaves → tree depth 2, so every proof should have exactly 2 nodes
  for (const auto &tx : txs) {
    auto proof = tree.getProof(hashTxn(tx));
    EXPECT_EQ(proof.size(), 2);
  }

  // non-existent txn should return an empty proof
  auto fake = std::make_shared<CurrencyTxn>(); fake->amount = 999;
  auto emptyProof = tree.getProof(hashTxn(fake));
  EXPECT_TRUE(emptyProof.empty());
}

// ---------- getProof – odd number of leaves ----------

TEST(MerkleTest, GetProofOddTree) {
  auto tx1 = std::make_shared<CurrencyTxn>(); tx1->amount = 10;
  auto tx2 = std::make_shared<CurrencyTxn>(); tx2->amount = 20;
  auto tx3 = std::make_shared<CurrencyTxn>(); tx3->amount = 30;

  std::vector<std::shared_ptr<Txn>> txs = {tx1, tx2, tx3};
  MerkleTree tree(txs);

  // 3 leaves → padded to 4, depth 2, proof length should be 2
  for (const auto &tx : txs) {
    auto proof = tree.getProof(hashTxn(tx));
    EXPECT_EQ(proof.size(), 2);
  }

  // non-existent txn
  auto fake = std::make_shared<CurrencyTxn>(); fake->amount = 777;
  EXPECT_TRUE(tree.getProof(hashTxn(fake)).empty());
}

// ---------- verifyProof – success cases ----------

TEST(MerkleTest, VerifyProofSuccess) {
  // even tree (4 leaves)
  {
    auto tx1 = std::make_shared<CurrencyTxn>(); tx1->amount = 10;
    auto tx2 = std::make_shared<CurrencyTxn>(); tx2->amount = 20;
    auto tx3 = std::make_shared<CurrencyTxn>(); tx3->amount = 30;
    auto tx4 = std::make_shared<CurrencyTxn>(); tx4->amount = 40;

    std::vector<std::shared_ptr<Txn>> txs = {tx1, tx2, tx3, tx4};
    MerkleTree tree(txs);
    std::string root = tree.getRoot();

    for (const auto &tx : txs) {
      std::string txHash = hashTxn(tx);
      auto proof = tree.getProof(txHash);
      EXPECT_TRUE(MerkleTree::verifyProof(root, txHash, proof));
    }
  }

  // odd tree (3 leaves)
  {
    auto tx1 = std::make_shared<CurrencyTxn>(); tx1->amount = 100;
    auto tx2 = std::make_shared<CurrencyTxn>(); tx2->amount = 200;
    auto tx3 = std::make_shared<CurrencyTxn>(); tx3->amount = 300;

    std::vector<std::shared_ptr<Txn>> txs = {tx1, tx2, tx3};
    MerkleTree tree(txs);
    std::string root = tree.getRoot();

    for (const auto &tx : txs) {
      std::string txHash = hashTxn(tx);
      auto proof = tree.getProof(txHash);
      EXPECT_TRUE(MerkleTree::verifyProof(root, txHash, proof));
    }
  }
}

// ---------- verifyProof – failure cases ----------

TEST(MerkleTest, VerifyProofFailure) {
  auto tx1 = std::make_shared<CurrencyTxn>(); tx1->amount = 10;
  auto tx2 = std::make_shared<CurrencyTxn>(); tx2->amount = 20;
  auto tx3 = std::make_shared<CurrencyTxn>(); tx3->amount = 30;
  auto tx4 = std::make_shared<CurrencyTxn>(); tx4->amount = 40;

  std::vector<std::shared_ptr<Txn>> txs = {tx1, tx2, tx3, tx4};
  MerkleTree tree(txs);
  std::string root = tree.getRoot();

  std::string txHash = hashTxn(tx1);
  auto proof = tree.getProof(txHash);

  // 1. tampered transaction hash
  EXPECT_FALSE(MerkleTree::verifyProof(root, "tampered_data", proof));

  // 2. tampered proof node
  {
    auto bad = proof;
    bad[0].hash = "corrupted_hash";
    EXPECT_FALSE(MerkleTree::verifyProof(root, txHash, bad));
  }

  // 3. wrong root
  EXPECT_FALSE(MerkleTree::verifyProof("wrong_root", txHash, proof));

  // 4. empty proof against a multi-leaf tree
  {
    std::vector<MerkleTree::ProofNode> empty;
    EXPECT_FALSE(MerkleTree::verifyProof(root, txHash, empty));
  }

  // 5. proof from a different transaction should not verify for this one
  {
    std::string otherHash = hashTxn(tx3);
    auto otherProof = tree.getProof(otherHash);
    EXPECT_FALSE(MerkleTree::verifyProof(root, txHash, otherProof));
  }
}
