#include "../src/core/MerkleTree.h"
#include "../src/models/CurrencyTxn.h"
#include <gtest/gtest.h>

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
}
