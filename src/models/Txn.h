#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

class Txn {
    public:
        std::string id;
        std::string from;
        std::string signature;

        virtual ~Txn() = default;
        virtual nlohmann::json toJson() const = 0;
        virtual std::string getType() const = 0;
};