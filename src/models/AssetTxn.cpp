#include "./AssetTxn.h"
#include "../core/TxnFactory.h"

namespace {
    const bool registered = []() {
        TxnFactory::registerType("ASSET", AssetTxn::fromJson);
        return true;
    }();
}