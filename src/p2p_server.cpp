#include "p2p_server.h"
#include <iostream>

void P2pServer::onOpen(WS& conn) {
    std::lock_guard<std::mutex> lock(peersMutex);
    peers.insert(&conn);
    std::cout << "[WS] Peer connected. Total peers: " << peers.size() << "\n";
}

void P2pServer::onClose(WS& conn) {
    std::lock_guard<std::mutex> lock(peersMutex);
    peers.erase(&conn);
    std::cout << "[WS] Peer disconnected. Total peers: " << peers.size() << "\n";
}

void P2pServer::onMessage(WS& conn, const std::string& msg) {
    std::cout << "[WS] Received: " << msg << "\n";

    // For now: broadcast raw message to everyone else
    broadcast(msg);
}

void P2pServer::broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(peersMutex);

    for (auto* peer : peers) {
        peer->send_text(msg);
    }
}
