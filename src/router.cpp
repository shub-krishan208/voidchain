#include "router.h"
#include "TxnPool.h"
#include "chain.h"
#include "core/TxnFactory.h"
#include "miner.h"
#include "wallet.h"
#include <crow/http_response.h>
#include <crow/json.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

Router::Router(crow::SimpleApp &app, Blockchain &blockchain, Wallet &wallet,
               TxnPool &pool, Miner &miner)
    : blockchain_(blockchain), wallet_(wallet), pool_(pool), miner_(miner),
      app_(app) {}

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

          if (!pool_.addTxn(txn)) {
            return crow::response(400, "Invalid transaction");
          }

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
}
