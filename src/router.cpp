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
      res["blocks"][idx++] = block.toJson();
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
        res["message"] = "Block added successfully";
        res["data"] = data;

        Block new_block = blockchain_.getLatestBlock();
        res["new_block"] = new_block.toJson();

        return crow::response(200, res);
      });
}
