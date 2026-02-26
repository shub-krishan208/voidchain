#pragma once

#include "TxnPool.h"
#include "chain.h"

class Miner {
public:
  Miner(Blockchain &chain, TxnPool &pol);
  Block mine();

private:
  Blockchain &chain_;
  TxnPool &pool_;
};