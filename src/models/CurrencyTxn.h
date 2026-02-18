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
};