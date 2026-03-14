#ifndef VOID_ROUTER_H
#define VOID_ROUTER_H

#include <crow.h>

class Blockchain;
class Wallet;
class TxnPool;
class Miner;

class Router {
public:
    Router(crow::SimpleApp& app, Blockchain& blockchain, Wallet& wallet,
           TxnPool& pool, Miner& miner);
    void registerRoutes();

private:
    Blockchain& blockchain_;
    Wallet& wallet_;
    TxnPool& pool_;
    Miner& miner_;
    crow::SimpleApp& app_;
};

#endif // VOID_ROUTER_H