#include "./TxnFactory.h"
#include <memory>
#include <stdexcept>
#include <unordered_map>

std::unordered_map<std::string, TxnFactory::Creator>&
TxnFactory::registry() {
    static std::unordered_map<std::string, Creator> instance;
    return instance;
}

void TxnFactory::registerType(const std::string &type, Creator creator){
    registry()[type] = creator;
}

std::shared_ptr<Txn>
TxnFactory::createTxn(const nlohmann::json& j){
    if (!j.contains("type")) {
    throw std::runtime_error("Error: no 'type' field");
    }
    std::string type = j.at("type");
    auto it = registry().find(type);

    if(it == registry().end()) throw std::runtime_error("Error: unknown transaction type -> " + type);
    return it->second(j);
}