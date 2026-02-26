#pragma once

#include "models/Txn.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

class TxnPool {
public:
  bool addTxn(std::shared_ptr<Txn> txn);

  std::vector<std::shared_ptr<Txn>> getTxn() const;

  void remove(const std::string &id);
  void clear();

private:
  std::map<std::string, std::shared_ptr<Txn>> pool_;
  bool verifyTxn(const std::shared_ptr<Txn> &txn) const;
};