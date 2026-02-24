#include "p2p_server.h"
#include <iostream>

void P2pServer::onOpen(WS& conn, Blockchain& blockchain) {
    std::lock_guard<std::mutex> lock(peersMutex);
    peers.insert(&conn);
    std::cout << "[WS] Peer connected. Total peers: " << peers.size() << "\n";

    // chain logic
    sendChain(conn, blockchain);
}

void P2pServer::sendChain(WS& conn, Blockchain& blockchain) {
    crow::json::wvalue msg;
    msg["type"] = "CHAIN";
    
    int idx = 0;
    for (const auto &block : blockchain.getChain()) {
      msg["data"][idx++] = std::move(block.toJson());
    }

    conn.send_text(msg.dump());
}

void P2pServer::onClose(WS& conn) {
    std::lock_guard<std::mutex> lock(peersMutex);
    peers.erase(&conn);
    std::cout << "[WS] Peer disconnected. Total peers: " << peers.size() << "\n";
}

void P2pServer::onMessage(WS& conn, const std::string& data, Blockchain& blockchain) {
    std::cout << "[WS] Received: " << data << "\n";

    try
    {
        auto msg = crow::json::load(data);
        if(!msg) {
            std::cerr << "[WS] Invalid JSON message received.\n";
            conn.send_text(R"({"type":"ERROR","message":"Invalid JSON"})");
            return;
        }
        
        if(msg["type"].s() == "CHAIN") {
            std::cout << "[WS] Received CHAIN message.\n";
    
            std::vector<Block> newChain = blockchain.getChain();
            /**
             * TODO: add transactions to the block
            */
            newChain.push_back(Block::mineBlock(newChain.back(), std::vector<std::shared_ptr<Txn>>{}));
            crow::json::wvalue res;
            res["type"] = "CHAIN";
            res["message"] = "Chain updated. New length: " + std::to_string(newChain.size());
            if(blockchain.replaceBlockchain(newChain)) {
                std::cout << "[WS] Chain replaced. New chain length: " << blockchain.getChain().size() << "\n";
                broadcast(conn, res.dump()); // propagate the new chain to other peers
            }
            return;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "[WS] Error parsing message: " << e.what() << '\n';
        conn.send_text(R"({"type":"ERROR","message":"Error parsing message"})");
        return;
    }
    

    conn.send_text(R"({"type":"ERROR","message":"Unknown error"})");
}

void P2pServer::broadcast(WS& exception, const std::string& msg) {
    std::lock_guard<std::mutex> lock(peersMutex);

    for (auto* peer : peers) {
        if(peer == &exception) continue;
        peer->send_text(msg);
    }
}
