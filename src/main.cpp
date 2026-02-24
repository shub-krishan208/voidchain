#include "./utils/TimeUtils.h"
#include "block.h"
#include "chain.h"
#include "models/CurrencyTxn.h"
#include "p2p_server.h"
#include "router.h"
#include "wallet.h"

#include <iostream>
// #include <nlohmann/json.hpp>

// using json = nlohmann::json;
using str = std::string;
void printBlock(str title, Block b) {
  std::cout << title << "\n"
            << "Timestamp: " << getFormattedTimestamp(b.getTimestamp()) << "\n"
            << "Last Hash: " << b.getLastHash() << "\n"
            << "Hash: " << b.getHash() << "\n"
            << "Data: " << b.getMerkleRoot() << std::endl;
}

void printBlocks() {
  auto tx1 = std::make_shared<CurrencyTxn>();
  tx1->id = "tx001";
  tx1->from = "alice";
  tx1->to = "bob";
  tx1->amount = 50.0;
  tx1->signature = "sig_alice_001";

  auto tx2 = std::make_shared<CurrencyTxn>();
  tx2->id = "tx002";
  tx2->from = "bob";
  tx2->to = "charlie";
  tx2->amount = 25.0;
  tx2->signature = "sig_bob_002";

  std::vector<std::shared_ptr<Txn>> txns = {tx1, tx2};

  Block FirstBlock = Block::genesis();
  Block newBlock = Block::mineBlock(FirstBlock, txns);
  Block block = Block::mineBlock(newBlock, txns);
  printBlock("Genesis Block", FirstBlock);
  printBlock("Random Block", block);
  printBlock("Mined Block", newBlock);
}

void printChain() {
  Blockchain chain;
  printBlock("Chain initialised", chain.getLatestBlock());
  auto tx1 = std::make_shared<CurrencyTxn>();
  tx1->id = "tx001";
  tx1->from = "alice";
  tx1->to = "bob";
  tx1->amount = 50.0;
  tx1->signature = "sig_alice_001";

  auto tx2 = std::make_shared<CurrencyTxn>();
  tx2->id = "tx002";
  tx2->from = "bob";
  tx2->to = "charlie";
  tx2->amount = 25.0;
  tx2->signature = "sig_bob_002";

  std::vector<std::shared_ptr<Txn>> txns = {tx1, tx2};


  chain.addBlock(txns);
  printBlock("After adding first block", chain.getLatestBlock());
}

void printWallet() {
  Wallet w;
  auto sig = w.sign("Some data to sign");
  std::cout << "Wallet Address: " << w.getAddress() << std::endl
            << "Public Key: " << w.getPublicKey() << std::endl
            << "Signature verification: "
            << OpenSSLWrapper::verify(w.getPublicKey(), "Some data to sign",
                                      sig)
            << std::endl;
}

int main() {
  Blockchain bc;
  crow::SimpleApp app;

  P2pServer p2p;
  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onopen([&p2p, &bc](crow::websocket::connection &conn) {
        p2p.onOpen(conn, bc);
      })
      .onclose([&p2p](crow::websocket::connection &conn,
                      const std::string &reason,
                      uint16_t code) { p2p.onClose(conn); })
      .onmessage([&p2p, &bc](crow::websocket::connection &conn,
                             const std::string &data, bool is_binary) {
        if (!is_binary) {
          p2p.onMessage(conn, data, bc);
        }
      });

  Router router(app, bc);

  router.registerRoutes();
  printChain();
  printWallet();
  app.port(18169).multithreaded().run();
  // printBlocks();
  return 0;
}
