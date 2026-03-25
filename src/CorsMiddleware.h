#pragma once

#include <crow.h>

struct CorsMiddleware {
  struct context {};

  static void addHeaders(crow::response &res) {
    res.add_header("Access-Control-Allow-Origin", "*");
    res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    res.add_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  }

  void before_handle(crow::request &req, crow::response &res, context &) {
    addHeaders(res);
    if (req.method == crow::HTTPMethod::Options) {
      res.code = 204;
      res.end();
    }
  }

  void after_handle(crow::request &, crow::response &res, context &) {
    addHeaders(res);
  }
};
