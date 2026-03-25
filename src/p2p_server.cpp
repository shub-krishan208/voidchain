#include "p2p_server.h"
#include "TxnPool.h"
#include "core/State.h"
#include "core/TxnFactory.h"
#include <iostream>
#include <nlohmann/json.hpp>

void P2pServer::onOpen(WS &conn, Blockchain &blockchain) {
  {
    std::lock_guard<std::mutex> lock(peersMutex);
    peers.insert(&conn);
    std::cout << "[WS] Peer connected. Total peers: " << peers.size() << "\n";
  }

  sendChain(conn, blockchain);
}

void P2pServer::setOutboundBroadcaster(
    std::function<void(const std::string &)> cb) {
  outboundBroadcaster_ = std::move(cb);
}

std::string P2pServer::makeChainMessage(const Blockchain &blockchain) {
  crow::json::wvalue msg;
  msg["type"] = "CHAIN";
  std::vector<crow::json::wvalue> chainJson;
  for (const auto &block : blockchain.getChain()) {
    chainJson.push_back(block.toJson());
  }
  msg["data"] = std::move(chainJson);
  return msg.dump();
}

std::string P2pServer::makeTransactionMessage(const Txn &txn) {
  crow::json::wvalue msg;
  msg["type"] = "TRANSACTION";
  msg["data"] = crow::json::load(txn.toJson().dump());
  return msg.dump();
}

std::string P2pServer::makeBlockMessage(const Block &block) {
  crow::json::wvalue msg;
  msg["type"] = "BLOCK";
  msg["data"] = block.toJson();
  return msg.dump();
}

void P2pServer::sendChain(WS &conn, Blockchain &blockchain) {
  conn.send_text(makeChainMessage(blockchain));
}

void P2pServer::onClose(WS &conn) {
  std::lock_guard<std::mutex> lock(peersMutex);
  peers.erase(&conn);
  std::cout << "[WS] Peer disconnected. Total peers: " << peers.size() << "\n";
}

bool P2pServer::processMessage(const std::string &data, Blockchain &blockchain,
                               TxnPool &pool) {
  try {
    auto msg = crow::json::load(data);
    if (!msg || !msg.has("type")) {
      return false;
    }

    const std::string type = msg["type"].s();
    if (type == "CHAIN") {
      if (!msg.has("data")) {
        return false;
      }

      std::vector<Block> incomingChain;
      const auto &chainData = msg["data"];
      incomingChain.reserve(chainData.size());
      for (size_t i = 0; i < chainData.size(); ++i) {
        if (i == 0) {
          const Block genesis = Block::genesis();
          const auto &g = chainData[i];
          if (g["hash"].s() != genesis.getHash() ||
              g["last_hash"].s() != genesis.getLastHash() ||
              g["merkle_root"].s() != genesis.getMerkleRoot() ||
              g["timestamp"].i() != genesis.getTimestamp() ||
              g["nonce"].i() != genesis.getNonce() ||
              g["difficulty"].i() != genesis.getDifficulty()) {
            return false;
          }
          incomingChain.push_back(genesis);
        } else {
          incomingChain.push_back(Block::fromJson(chainData[i]));
        }
      }

      try {
        StateValidationResult chainStateResult =
            State::validateFullChain(incomingChain);
        if (!chainStateResult.ok) {
          return false;
        }
        return blockchain.replaceBlockchain(incomingChain);
      } catch (const std::exception &) {
        return false;
      }
    }

    if (type == "TRANSACTION") {
      if (!msg.has("data")) {
        return false;
      }
      std::string txStr = crow::json::wvalue(msg["data"]).dump();
      nlohmann::json txJson = nlohmann::json::parse(txStr);
      auto txn = TxnFactory::createTxn(txJson);
      StateValidationResult txStateResult = State::validatePoolAdmission(
          txn, blockchain.getChain(), pool.getTxn());
      if (!txStateResult.ok) {
        return false;
      }
      return pool.addTxn(txn);
    }

    if (type == "BLOCK") {
      if (!msg.has("data")) {
        return false;
      }

      Block block = Block::fromJson(msg["data"]);
      StateValidationResult blockStateResult =
          State::validateBlockAppend(block, blockchain.getChain());
      if (!blockStateResult.ok) {
        return false;
      }
      if (!blockchain.addBlock(block)) {
        return false;
      }

      const auto &txs = msg["data"]["transactions"];
      for (size_t i = 0; i < txs.size(); ++i) {
        if (txs[i].has("id")) {
          pool.remove(txs[i]["id"].s());
        }
      }

      return true;
    }
  } catch (const std::exception &e) {
    std::cerr << "[WS] Error parsing message: " << e.what() << "\n";
    return false;
  }

  return false;
}

void P2pServer::onMessage(WS &conn, const std::string &data,
                          Blockchain &blockchain, TxnPool &pool) {
  std::cout << "[WS] Received: " << data << "\n";
  bool applied = processMessage(data, blockchain, pool);
  if (!applied) {
    conn.send_text(R"({"type":"ERROR","message":"Message rejected"})");
    return;
  }

  broadcast(conn, data);
  if (outboundBroadcaster_) {
    outboundBroadcaster_(data);
  }
}

bool P2pServer::onPeerMessage(const std::string &msg, Blockchain &blockchain,
                              TxnPool &pool) {
  bool applied = processMessage(msg, blockchain, pool);
  if (!applied) {
    return false;
  }

  broadcastMessage(msg);
  if (outboundBroadcaster_) {
    outboundBroadcaster_(msg);
  }
  return true;
}

void P2pServer::broadcast(WS &exception, const std::string &msg) {
  std::lock_guard<std::mutex> lock(peersMutex);
  for (auto *peer : peers) {
    if (peer == &exception)
      continue;
    peer->send_text(msg);
  }
}

void P2pServer::broadcastMessage(const std::string &msg) {
  std::lock_guard<std::mutex> lock(peersMutex);
  for (auto *peer : peers) {
    peer->send_text(msg);
  }
}
