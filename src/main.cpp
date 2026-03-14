#include "TxnPool.h"
#include "chain.h"
#include "core/TxnFactory.h"
#include "miner.h"
#include "models/AssetTxn.h"
#include "models/CurrencyTxn.h"
#include "p2p_server.h"
#include "router.h"
#include "wallet.h"

int main() {
  TxnFactory::registerType("CURRENCY", CurrencyTxn::fromJson);
  TxnFactory::registerType("ASSET", AssetTxn::fromJson);

  Blockchain blockchain;
  Wallet wallet;
  TxnPool pool;
  Miner miner(blockchain, pool);

  crow::SimpleApp app;

  P2pServer p2p;
  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onopen([&p2p, &blockchain](crow::websocket::connection &conn) {
        p2p.onOpen(conn, blockchain);
      })
      .onclose([&p2p](crow::websocket::connection &conn,
                      const std::string &reason,
                      uint16_t code) { p2p.onClose(conn); })
      .onmessage([&p2p, &blockchain](crow::websocket::connection &conn,
                             const std::string &data, bool is_binary) {
        if (!is_binary) {
          p2p.onMessage(conn, data, blockchain);
        }
      });

  Router router(app, blockchain, wallet, pool, miner);
  router.registerRoutes();

  app.port(18169).multithreaded().run();
  return 0;
}
