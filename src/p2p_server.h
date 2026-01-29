#pragma once
#include <crow.h>
#include <unordered_set>
#include <mutex>

class P2pServer {
public:
    using WS = crow::websocket::connection;

    void onOpen(WS& conn);
    void onClose(WS& conn);
    void onMessage(WS& conn, const std::string& msg);

    void broadcast(const std::string& msg);

private:
    std::unordered_set<WS*> peers;
    std::mutex peersMutex;
};