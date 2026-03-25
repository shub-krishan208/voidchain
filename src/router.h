#ifndef VOID_ROUTER_H
#define VOID_ROUTER_H

#include "CorsMiddleware.h"
#include <crow.h>

class Blockchain;
class Wallet;
class TxnPool;
class Miner;
class P2pServer;
class PeerClient;

class Router {
public:
    Router(crow::App<CorsMiddleware>& app, Blockchain& blockchain, Wallet& wallet,
           TxnPool& pool, Miner& miner, P2pServer& p2p, PeerClient& peerClient);
    void registerRoutes();

private:
    Blockchain& blockchain_;
    Wallet& wallet_;
    TxnPool& pool_;
    Miner& miner_;
    P2pServer& p2p_;
    PeerClient& peerClient_;
    crow::App<CorsMiddleware>& app_;
};

#endif // VOID_ROUTER_H