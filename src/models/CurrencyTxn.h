#pragma once
#include "Txn.h"

struct CurrencyTxn : public Txn {
    std::string to;
    double amount;

    nlohmann::json toJson() const override {
        return {
            {"type", "CURRENCY"},
            {"id", id},
            {"from", from},
            {"to", to},
            {"amount", amount},
            {"signature", signature}
        };
    }

    std::string getType() const override {
        return "CURRENCY";
    }

    static std::shared_ptr<Txn> fromJson(const nlohmann::json& j) {
        auto txn = std::make_shared<CurrencyTxn>();
        txn->id = j.at("id");
        txn->from = j.at("from");
        txn->to = j.at("to");
        txn->amount = j.at("amount");
        txn->signature = j.at("signature");
        return txn;
    }
};