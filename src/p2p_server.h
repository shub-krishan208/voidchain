#pragma once
#include <crow.h>
#include <unordered_set>
#include <mutex>
#include "chain.h"

class P2pServer {
public:
    using WS = crow::websocket::connection;

    void onOpen(WS& conn, Blockchain& blockchain);
    void onClose(WS& conn);
    void onMessage(WS& conn, const std::string& msg, Blockchain& blockchain);

    void broadcast(WS& exception, const std::string& msg);
    void sendChain(WS& conn, Blockchain& blockchain);
private:
    std::unordered_set<WS*> peers;
    std::mutex peersMutex;
};

enum class MSGTYPES {
    CHAIN,
    CURRENCY,
    ASSET,
    ERROR
};