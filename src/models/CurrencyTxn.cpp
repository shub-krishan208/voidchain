#include "./CurrencyTxn.h"
#include "../core/TxnFactory.h"

namespace {
    const bool registered = []() {
        TxnFactory::registerType("CURRENCY", CurrencyTxn::fromJson);
        return true;
    }();
}