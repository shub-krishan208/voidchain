#pragma once
#include <crow.h>
#include "chain.h"
#include "models/Txn.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>

class TxnPool;

class P2pServer {
public:
  using WS = crow::websocket::connection;

  void onOpen(WS &conn, Blockchain &blockchain);
  void onClose(WS &conn);
  void onMessage(WS &conn, const std::string &msg, Blockchain &blockchain,
                 TxnPool &pool);
  bool onPeerMessage(const std::string &msg, Blockchain &blockchain,
                     TxnPool &pool);

  void setOutboundBroadcaster(std::function<void(const std::string &)> cb);
  void broadcast(WS &exception, const std::string &msg);
  void broadcastMessage(const std::string &msg);
  void sendChain(WS &conn, Blockchain &blockchain);

  static std::string makeChainMessage(const Blockchain &blockchain);
  static std::string makeTransactionMessage(const Txn &txn);
  static std::string makeBlockMessage(const Block &block);

private:
  bool processMessage(const std::string &data, Blockchain &blockchain,
                      TxnPool &pool);

  std::unordered_set<WS *> peers;
  std::mutex peersMutex;
  std::function<void(const std::string &)> outboundBroadcaster_;
};

enum class MSGTYPES {
  CHAIN,
  TRANSACTION,
  BLOCK,
  ERROR
};