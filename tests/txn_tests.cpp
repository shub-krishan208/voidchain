#include <gtest/gtest.h>
#include "../src/models/AssetTxn.h"
#include "../src/models/CurrencyTxn.h"

TEST(TxnTest, AssetTxnToJson) {
    AssetTxn assetTxn;
    assetTxn.id = "txn1";
    assetTxn.from = "Alice";
    assetTxn.to = "Bob";
    assetTxn.itemId = "item123";
    assetTxn.meta = "A rare item";
    assetTxn.signature = "signature123";

    nlohmann::json expectedJson = {
        {"type", "ASSET"},
        {"id", "txn1"},
        {"from", "Alice"},
        {"to", "Bob"},
        {"itemId", "item123"},
        {"meta", "A rare item"},
        {"signature", "signature123"}
    };

    EXPECT_EQ(assetTxn.toJson(), expectedJson);
}

TEST(TxnTest, CurrencyTxnToJson) {
    CurrencyTxn currencyTxn;
    currencyTxn.id = "txn2";
    currencyTxn.from = "Charlie";
    currencyTxn.to = "Dave";
    currencyTxn.amount = 100.50;
    currencyTxn.signature = "signature456";

    nlohmann::json expectedJson = {
        {"type", "CURRENCY"},
        {"id", "txn2"},
        {"from", "Charlie"},
        {"to", "Dave"},
        {"amount", 100.50},
        {"signature", "signature456"}
    };

    EXPECT_EQ(currencyTxn.toJson(), expectedJson);
}

TEST(TxnTest, PolymorphicToJson) {
    std::vector<std::shared_ptr<Txn>> txs;

    auto moneyTx = std::make_shared<CurrencyTxn>();
    moneyTx->id = "1";
    moneyTx->from = "Alice";
    moneyTx->to = "Bob";
    moneyTx->amount = 100;

    auto assetTx = std::make_shared<AssetTxn>();
    assetTx->id = "2";
    assetTx->from = "Alice";
    assetTx->to = "Bob";
    assetTx->itemId = "IRON_SWORD";
    assetTx->meta = "Level 5";

    txs.push_back(moneyTx);
    txs.push_back(assetTx);

    auto json1 = txs[0]->toJson();
    auto json2 = txs[1]->toJson();

    EXPECT_EQ(json1["type"], "CURRENCY");
    EXPECT_EQ(json2["type"], "ASSET");
}
