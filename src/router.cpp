#include "router.h"
#include "TxnPool.h"
#include "chain.h"
#include "core/MerkleTree.h"
#include "core/State.h"
#include "core/TxnFactory.h"
#include "miner.h"
#include "network/PeerClient.h"
#include "p2p_server.h"
#include "utils/hashing.h"
#include "wallet.h"
#include <crow/http_response.h>
#include <crow/json.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <set>
#include <stdexcept>

Router::Router(crow::SimpleApp &app, Blockchain &blockchain, Wallet &wallet,
               TxnPool &pool, Miner &miner, P2pServer &p2p,
               PeerClient &peerClient)
    : blockchain_(blockchain), wallet_(wallet), pool_(pool), miner_(miner),
      p2p_(p2p), peerClient_(peerClient), app_(app) {}

void Router::registerRoutes() {
  CROW_ROUTE(app_, "/health")([this]() {
    crow::json::wvalue res;
    res["status"] = "OK";
    return res;
  });

  CROW_ROUTE(app_, "/blocks")([this]() {
    crow::json::wvalue res;

    int idx = 0;
    for (const auto &block : blockchain_.getChain()) {
      res["blocks"][idx++] = block.toJson();
    }
    return res;
  });

  CROW_ROUTE(app_, "/headers")([this]() {
    crow::json::wvalue res;
    std::vector<Block> chainSnapshot;
    {
      std::lock_guard<std::mutex> lock(blockchain_.chainMutex);
      chainSnapshot = blockchain_.getChain();
    }

    int idx = 0;
    for (const auto &block : chainSnapshot) {
      auto header = block.headerToJson();
      header["height"] = idx;
      res["headers"][idx++] = std::move(header);
    }
    return res;
  });

  CROW_ROUTE(app_, "/proof")
      .methods(crow::HTTPMethod::GET)([this](const crow::request &req) {
        const char *txId = req.url_params.get("txId");
        if (txId == nullptr || std::string(txId).empty()) {
          return crow::response(400, "Missing required query param: txId");
        }

        try {
          std::vector<Block> chainSnapshot;
          {
            std::lock_guard<std::mutex> lock(blockchain_.chainMutex);
            chainSnapshot = blockchain_.getChain();
          }

          size_t blockIndex = 0;
          std::shared_ptr<Txn> foundTxn;
          if (!Blockchain::findTransactionInChain(chainSnapshot,
                                                  std::string(txId), blockIndex,
                                                  foundTxn)) {
            return crow::response(404, "Transaction not found");
          }

          const auto &containingBlock = chainSnapshot[blockIndex];
          Hasher txHasher;
          txHasher.add(foundTxn->toJson().dump());
          const std::string txHash = txHasher.finish();

          MerkleTree tree(containingBlock.getTransactions());
          const auto proof = tree.getProof(txHash);

          crow::json::wvalue res;
          res["txId"] = std::string(txId);
          res["txHash"] = txHash;
          res["txData"] = crow::json::load(foundTxn->toJson().dump());
          res["root"] = containingBlock.getMerkleRoot();
          res["block"] = containingBlock.headerToJson();
          res["block"]["height"] = static_cast<int>(blockIndex);

          std::vector<crow::json::wvalue> proofNodes;
          proofNodes.reserve(proof.size());
          for (const auto &node : proof) {
            crow::json::wvalue proofNode;
            proofNode["hash"] = node.hash;
            proofNode["isLeft"] = node.isLeft;
            proofNodes.push_back(std::move(proofNode));
          }
          res["proof"] = std::move(proofNodes);
          return crow::response(200, res);
        } catch (const std::exception &e) {
          return crow::response(500, e.what());
        }
      });

  CROW_ROUTE(app_, "/state")([this]() {
    try {
      const DerivedState state = State::deriveFromChainOrThrow(blockchain_.getChain());
      nlohmann::json payload;
      payload["balances"] = state.balances;
      payload["owner_by_asset"] = state.ownerByAsset;
      payload["assets_by_owner"] = state.assetsByOwner;

      crow::json::wvalue res = crow::json::load(payload.dump());
      return crow::response(200, res);
    } catch (const std::exception &e) {
      return crow::response(500, e.what());
    }
  });

  CROW_ROUTE(app_, "/balance")
      .methods(crow::HTTPMethod::GET)([this](const crow::request &req) {
        const char *address = req.url_params.get("address");
        if (address == nullptr || std::string(address).empty()) {
          return crow::response(400, "Missing required query param: address");
        }

        try {
          const DerivedState state =
              State::deriveFromChainOrThrow(blockchain_.getChain());
          crow::json::wvalue res;
          res["address"] = std::string(address);
          res["balance"] = State::getBalance(state, std::string(address));
          return crow::response(200, res);
        } catch (const std::exception &e) {
          return crow::response(500, e.what());
        }
      });

  CROW_ROUTE(app_, "/owner")
      .methods(crow::HTTPMethod::GET)([this](const crow::request &req) {
        const char *itemId = req.url_params.get("itemId");
        if (itemId == nullptr || std::string(itemId).empty()) {
          return crow::response(400, "Missing required query param: itemId");
        }

        try {
          const DerivedState state =
              State::deriveFromChainOrThrow(blockchain_.getChain());
          const std::string owner = State::getOwner(state, std::string(itemId));
          if (owner.empty()) {
            return crow::response(404, "Asset owner not found");
          }

          crow::json::wvalue res;
          res["itemId"] = std::string(itemId);
          res["owner"] = owner;
          return crow::response(200, res);
        } catch (const std::exception &e) {
          return crow::response(500, e.what());
        }
      });

  CROW_ROUTE(app_, "/assets")
      .methods(crow::HTTPMethod::GET)([this](const crow::request &req) {
        const char *address = req.url_params.get("address");
        if (address == nullptr || std::string(address).empty()) {
          return crow::response(400, "Missing required query param: address");
        }

        try {
          const DerivedState state =
              State::deriveFromChainOrThrow(blockchain_.getChain());
          const std::set<std::string> assets =
              State::getAssets(state, std::string(address));

          crow::json::wvalue res;
          std::vector<crow::json::wvalue> assetList;
          for (const auto &assetId : assets) {
            assetList.push_back(assetId);
          }
          res["address"] = std::string(address);
          res["assets"] = std::move(assetList);
          return crow::response(200, res);
        } catch (const std::exception &e) {
          return crow::response(500, e.what());
        }
      });

  CROW_ROUTE(app_, "/pool")([this]() {
    crow::json::wvalue res;
    std::vector<crow::json::wvalue> txnsJson;
    for (const auto &txn : pool_.getTxn()) {
      txnsJson.push_back(crow::json::load(txn->toJson().dump()));
    }
    res["transactions"] = std::move(txnsJson);
    return res;
  });

  CROW_ROUTE(app_, "/transact")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        nlohmann::json body;
        try {
          body = nlohmann::json::parse(req.body);
        } catch (const std::exception &) {
          return crow::response(400, "Invalid JSON body");
        }

        try {
          if (!body.contains("id")) {
            body["id"] = "";
          }
          if (!body.contains("from")) {
            body["from"] = "";
          }
          if (!body.contains("signature")) {
            body["signature"] = "";
          }

          auto txn = TxnFactory::createTxn(body);
          wallet_.signTxn(*txn);

          const StateValidationResult stateResult = State::validatePoolAdmission(
              txn, blockchain_.getChain(), pool_.getTxn());
          if (!stateResult.ok) {
            return crow::response(400, stateResult.error);
          }

          if (!pool_.addTxn(txn)) {
            return crow::response(400, "Invalid transaction");
          }

          const std::string msg = P2pServer::makeTransactionMessage(*txn);
          p2p_.broadcastMessage(msg);
          peerClient_.broadcast(msg);

          crow::json::wvalue res;
          res["message"] = "Transaction added to pool";
          res["transaction"] = crow::json::load(txn->toJson().dump());
          return crow::response(201, res);
        } catch (const std::exception &e) {
          return crow::response(400, e.what());
        }
      });

  CROW_ROUTE(app_, "/mine")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        (void)req;
        try {
          Block newBlock = miner_.mine();
          const std::string msg = P2pServer::makeBlockMessage(newBlock);
          p2p_.broadcastMessage(msg);
          peerClient_.broadcast(msg);

          crow::json::wvalue res;
          res["message"] = "Block mined successfully";
          res["new_block"] = newBlock.toJson();
          return crow::response(200, res);
        } catch (const std::runtime_error &e) {
          return crow::response(400, e.what());
        } catch (const std::exception &e) {
          return crow::response(500, e.what());
        }
      });

  CROW_ROUTE(app_, "/peer/message")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        try {
          auto parsed = nlohmann::json::parse(req.body);
          (void)parsed;
        } catch (const std::exception &) {
          return crow::response(400, "Invalid peer message JSON");
        }

        const bool applied = p2p_.onPeerMessage(req.body, blockchain_, pool_);
        crow::json::wvalue res;
        res["applied"] = applied;
        return crow::response(200, res);
      });
}
