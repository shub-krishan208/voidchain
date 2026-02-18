#pragma once
#include "Txn.h"

struct AssetTxn : public Txn {
    std::string to;
    std::string itemId;
    std::string meta;

    nlohmann::json toJson() const override {
        return {
            {"type", "ASSET"},
            {"id", id},
            {"from", from},
            {"to", to},
            {"itemId", itemId},
            {"meta", meta},
            {"signature", signature}
        };
    }

    std::string getType() const override {
        return "ASSET";
    }
};