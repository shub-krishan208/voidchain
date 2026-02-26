#pragma once
#include "../models/Txn.h"
#include "../models/CurrencyTxn.h"
#include "../models/AssetTxn.h"
#include <asio/prefer.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

class TxnFactory {
    public:
    using Creator = std::function<std::shared_ptr<Txn>(const nlohmann::json&)>;

    static std::shared_ptr<Txn> createTxn(const nlohmann::json& j);
    static void registerType(const std::string& type, Creator creator);

    private:
    static std::unordered_map<std::string, Creator>& registry();
};