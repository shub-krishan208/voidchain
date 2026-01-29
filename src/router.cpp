#include "router.h"
#include "chain.h"
#include <crow/http_response.h>
#include <crow/json.h>
#include <iostream>
#include <ostream>
#include <utility>

Router::Router(crow::SimpleApp &app, Blockchain &blockchain)
    : app_(app), blockchain_(blockchain) {}

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
      crow::json::wvalue bjson;
      bjson["timestamp"] = block.getTimestamp();
      bjson["hash"] = block.getHash();
      bjson["last_hash"] = block.getLastHash();
      bjson["data"] = block.getData();

      res["blocks"][idx++] = std::move(bjson);
    }
    return res;
  });

  // making an endpoint /mine to add a new block to the blockchain
  CROW_ROUTE(app_, "/mine")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        auto body_params = crow::json::load(req.body);

        if (!body_params) {
          return crow::response(400, "Invalid json!");
        }
        if (!body_params.has("data")) {
          return crow::response(400, "Missing data field");
        }

        std::string data = body_params["data"].s();

        blockchain_.addBlock(data);
        crow::json::wvalue res;
        res["message"] = "Block added successfullt";
        res["data"] = data;

        res["new_block"] = crow::json::wvalue();
        Block new_block = blockchain_.getLatestBlock();

        res["new_block"]["timestamp"] = new_block.getTimestamp();
        res["new_block"]["lastHash"] = new_block.getLastHash();
        res["new_block"]["hash"] = new_block.getHash();
        res["new_block"]["data"] = new_block.getData();

        return crow::response(200, res);
      });
}
