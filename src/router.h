#ifndef VOID_ROUTER_H
#define VOID_ROUTER_H

#include <crow.h>

class Blockchain;

class Router {
public:
    Router(crow::SimpleApp& app, Blockchain& blockchain);
    void registerRoutes();

private:
    Blockchain& blockchain_;
    crow::SimpleApp& app_;
};

#endif // VOID_ROUTER_H