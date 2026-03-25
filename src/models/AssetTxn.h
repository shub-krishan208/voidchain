#pragma once
#include "Txn.h"

struct AssetTxn : public Txn {
  std::string to;
  std::string itemId;
  std::string meta;

  nlohmann::json toJson() const override {
    auto j = toSignableJson();
    j["senderPubKey"] = senderPubKey;
    j["signature"] = signature;
    return j;
  }

  std::string getType() const override { return "ASSET"; }

  nlohmann::json toSignableJson() const override {
    return {{"type", "ASSET"}, {"id", id},         {"from", from},
            {"to", to},        {"itemId", itemId}, {"meta", meta}};
  }

  static std::shared_ptr<Txn> fromJson(const nlohmann::json &j) {
    auto txn = std::make_shared<AssetTxn>();
    txn->id = j.at("id");
    txn->from = j.at("from");
    txn->senderPubKey = j.value("senderPubKey", "");
    txn->to = j.at("to");
    txn->itemId = j.at("itemId");
    txn->meta = j.at("meta");
    txn->signature = j.at("signature");
    return txn;
  }
};