#include "TxnPool.h"
#include "chain.h"
#include "core/TxnFactory.h"
#include "miner.h"
#include "models/AssetTxn.h"
#include "models/CurrencyTxn.h"
#include "network/PeerClient.h"
#include "p2p_server.h"
#include "router.h"
#include "utils/env.h"
#include "wallet.h"
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> parsePeers(const std::string &raw) {
  std::vector<std::string> peers;
  std::stringstream ss(raw);
  std::string peer;
  while (std::getline(ss, peer, ',')) {
    if (!peer.empty()) {
      peers.push_back(peer);
    }
  }
  return peers;
}
} // namespace

int main() {
  loadEnv(".env");
  TxnFactory::registerType("CURRENCY", CurrencyTxn::fromJson);
  TxnFactory::registerType("ASSET", AssetTxn::fromJson);

  Blockchain blockchain;
  Wallet wallet;
  TxnPool pool;
  Miner miner(blockchain, pool, wallet);

  crow::App<CorsMiddleware> app;

  P2pServer p2p;

  PeerClient peerClient(
      [&p2p, &blockchain, &pool](const std::string &message) mutable {
        p2p.onPeerMessage(message, blockchain, pool);
      });
  p2p.setOutboundBroadcaster([&peerClient](const std::string &message) {
    peerClient.broadcast(message);
  });

  const char *peersEnv = std::getenv("PEERS");
  if (peersEnv != nullptr) {
    for (const auto &peer : parsePeers(peersEnv)) {
      peerClient.connectToPeer(peer);
    }
  }

  CROW_WEBSOCKET_ROUTE(app, "/ws")
      .onopen([&p2p, &blockchain](crow::websocket::connection &conn) {
        p2p.onOpen(conn, blockchain);
      })
      .onclose([&p2p](crow::websocket::connection &conn,
                      const std::string &reason,
                      uint16_t code) { p2p.onClose(conn); })
      .onmessage([&p2p, &blockchain, &pool](crow::websocket::connection &conn,
                                            const std::string &data,
                                            bool is_binary) {
        if (!is_binary) {
          p2p.onMessage(conn, data, blockchain, pool);
        }
      });

  Router router(app, blockchain, wallet, pool, miner, p2p, peerClient);
  router.registerRoutes();

  int port = 18169;
  if (const char *portEnv = std::getenv("PORT")) {
    std::cout << "Env Loaded ...\n";
    port = std::atoi(portEnv);
  }
  std::cout << "PORT: " << port << "\n";
  app.port(static_cast<uint16_t>(port)).multithreaded().run();
  return 0;
}
