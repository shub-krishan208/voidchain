#include <gtest/gtest.h>
#include <memory>
#include "../src/utils/hashing.h"
#include "../src/models/CurrencyTxn.h"


// To check of the has changes if order is reversed
TEST(HashingTest, TestMerkleHash) {
    Hasher h, k;
    auto tx1 = std::make_shared<CurrencyTxn>();
    tx1->amount = 10;
    auto tx2 = std::make_shared<CurrencyTxn>();
    tx2->amount = 99;
    
    auto hash1 = tx1->toJson().dump();
    auto hash2 = tx2->toJson().dump();
    h.add(hash1);
    h.add(hash2);

    k.add(hash2);
    k.add(hash1);

    auto root1 = h.finish();
    auto root2 = k.finish();
    EXPECT_NE(root1, root2);
}