#pragma once

#include "TxnPool.h"
#include "chain.h"
#include "wallet.h"

class Miner {
public:
  static constexpr double MINING_REWARD = 50.0;

  Miner(Blockchain &chain, TxnPool &pool, Wallet &wallet);
  Block mine();

private:
  Blockchain &chain_;
  TxnPool &pool_;
  Wallet &wallet_;
};