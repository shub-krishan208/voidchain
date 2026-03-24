#include "router.h"
#include "TxnPool.h"
#include "chain.h"
#include "core/MerkleTree.h"
#include "core/State.h"
#include "core/TxnFactory.h"
#include "miner.h"
#include "models/AssetTxn.h"
#include "models/CurrencyTxn.h"
#include "network/PeerClient.h"
#include "p2p_server.h"
#include "utils/hashing.h"
#include "utils/hex.h"
#include "utils/uuid.h"
#include "wallet.h"

#include <crow/http_response.h>
#include <crow/json.h>
#include <nlohmann/json.hpp>

#include <cctype>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr size_t kDefaultRecentTxCount = 20;

std::string deriveAddressPem(EVP_PKEY *keyPair) {
  if (!keyPair) {
    return {};
  }

  EVP_PKEY *pub = OpenSSLWrapper::extractPublicKey(keyPair);
  if (!pub) {
    return {};
  }

  std::string pem = OpenSSLWrapper::publicKeyToPEM(pub);
  EVP_PKEY_free(pub);
  return pem;
}

bool transactionTouchesAddress(const std::shared_ptr<Txn> &txn,
                               const std::string &address) {
  if (!txn) {
    return false;
  }

  if (txn->from == address) {
    return true;
  }

  if (auto currency = std::dynamic_pointer_cast<CurrencyTxn>(txn)) {
    return currency->to == address;
  }

  if (auto asset = std::dynamic_pointer_cast<AssetTxn>(txn)) {
    return asset->to == address;
  }

  return false;
}

crow::json::wvalue transactionSummary(const std::shared_ptr<Txn> &txn,
                                      size_t blockHeight,
                                      int64_t blockTimestamp) {
  crow::json::wvalue item;
  item["txId"] = txn->id;
  item["type"] = txn->getType();
  item["from"] = txn->from;
  item["blockHeight"] = static_cast<int>(blockHeight);
  item["timestamp"] = blockTimestamp;

  if (auto currency = std::dynamic_pointer_cast<CurrencyTxn>(txn)) {
    item["to"] = currency->to;
    item["amount"] = currency->amount;
  } else if (auto asset = std::dynamic_pointer_cast<AssetTxn>(txn)) {
    item["to"] = asset->to;
    item["itemId"] = asset->itemId;
    item["meta"] = asset->meta;
  }

  return item;
}

std::vector<crow::json::wvalue>
collectTransactionsForAddress(const std::vector<Block> &chain,
                              const std::string &address,
                              size_t maxCount = 0) {
  std::vector<crow::json::wvalue> out;

  for (size_t blockIdx = chain.size(); blockIdx > 0; --blockIdx) {
    const size_t height = blockIdx - 1;
    const auto &block = chain[height];

    for (const auto &txn : block.getTransactions()) {
      if (!transactionTouchesAddress(txn, address)) {
        continue;
      }

      out.push_back(transactionSummary(txn, height, block.getTimestamp()));
      if (maxCount > 0 && out.size() >= maxCount) {
        return out;
      }
    }
  }

  return out;
}

std::string trimWhitespace(std::string value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start])) != 0) {
    ++start;
  }

  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
    --end;
  }

  return value.substr(start, end - start);
}
} // namespace

Router::Router(crow::App<CorsMiddleware> &app, Blockchain &blockchain,
               Wallet &wallet, TxnPool &pool, Miner &miner, P2pServer &p2p,
               PeerClient &peerClient)
    : blockchain_(blockchain), wallet_(wallet), pool_(pool), miner_(miner),
      p2p_(p2p), peerClient_(peerClient), app_(app) {}

void Router::registerRoutes() {
  CROW_ROUTE(app_, "/health")([this]() {
    crow::json::wvalue res;
    res["status"] = "OK";
    return res;
  });

  CROW_ROUTE(app_, "/wallet/generate")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        (void)req;

        EVP_PKEY *keyPair = OpenSSLWrapper::generateKeyPair();
        if (!keyPair) {
          return crow::response(500, "Failed to generate wallet keypair");
        }

        const std::string address = deriveAddressPem(keyPair);
        const std::string secretKey = OpenSSLWrapper::privateKeyToHex(keyPair);
        EVP_PKEY_free(keyPair);

        if (address.empty() || secretKey.empty()) {
          return crow::response(500, "Failed to derive wallet materials");
        }

        crow::json::wvalue res;
        res["address"] = address;
        res["secretKey"] = secretKey;
        return crow::response(201, res);
      });

  CROW_ROUTE(app_, "/wallet/recover")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        nlohmann::json body;
        try {
          body = nlohmann::json::parse(req.body);
        } catch (const std::exception &) {
          return crow::response(400, "Invalid JSON body");
        }

        if (!body.contains("secretKey") || !body["secretKey"].is_string()) {
          return crow::response(400, "Missing or invalid secretKey");
        }

        const std::string secretKey = body["secretKey"].get<std::string>();
        EVP_PKEY *keyPair = OpenSSLWrapper::keyPairFromPrivateHex(secretKey);
        if (!keyPair) {
          return crow::response(400, "Invalid secretKey");
        }

        const std::string address = deriveAddressPem(keyPair);
        EVP_PKEY_free(keyPair);

        if (address.empty()) {
          return crow::response(500, "Failed to recover wallet address");
        }

        crow::json::wvalue res;
        res["address"] = address;
        return crow::response(200, res);
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
      const DerivedState state =
          State::deriveFromChainOrThrow(blockchain_.getChain());
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
          const std::string addressStr = trimWhitespace(std::string(address));
          if (addressStr.empty()) {
            return crow::response(400, "Missing required query param: address");
          }

          const DerivedState state =
              State::deriveFromChainOrThrow(blockchain_.getChain());
          crow::json::wvalue res;
          res["address"] = addressStr;
          res["balance"] = State::getBalance(state, addressStr);
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
          const std::string addressStr = trimWhitespace(std::string(address));
          if (addressStr.empty()) {
            return crow::response(400, "Missing required query param: address");
          }

          const DerivedState state =
              State::deriveFromChainOrThrow(blockchain_.getChain());
          const std::set<std::string> assets = State::getAssets(state, addressStr);

          crow::json::wvalue res;
          std::vector<crow::json::wvalue> assetList;
          for (const auto &assetId : assets) {
            assetList.push_back(assetId);
          }
          res["address"] = addressStr;
          res["assets"] = std::move(assetList);
          return crow::response(200, res);
        } catch (const std::exception &e) {
          return crow::response(500, e.what());
        }
      });

  CROW_ROUTE(app_, "/transactions")
      .methods(crow::HTTPMethod::GET)([this](const crow::request &req) {
        const char *address = req.url_params.get("address");
        if (address == nullptr || std::string(address).empty()) {
          return crow::response(400, "Missing required query param: address");
        }

        const std::string addressStr = trimWhitespace(std::string(address));
        if (addressStr.empty()) {
          return crow::response(400, "Missing required query param: address");
        }

        std::vector<Block> chainSnapshot;
        {
          std::lock_guard<std::mutex> lock(blockchain_.chainMutex);
          chainSnapshot = blockchain_.getChain();
        }

        crow::json::wvalue res;
        res["address"] = addressStr;
        res["transactions"] =
            collectTransactionsForAddress(chainSnapshot, addressStr);
        return crow::response(200, res);
      });

  CROW_ROUTE(app_, "/wallet/info")
      .methods(crow::HTTPMethod::GET)([this](const crow::request &req) {
        const char *address = req.url_params.get("address");
        if (address == nullptr || std::string(address).empty()) {
          return crow::response(400, "Missing required query param: address");
        }

        try {
          std::vector<Block> chainSnapshot;
          {
            std::lock_guard<std::mutex> lock(blockchain_.chainMutex);
            chainSnapshot = blockchain_.getChain();
          }

          const std::string addressStr = trimWhitespace(std::string(address));
          if (addressStr.empty()) {
            return crow::response(400, "Missing required query param: address");
          }

          const DerivedState state = State::deriveFromChainOrThrow(chainSnapshot);
          const std::set<std::string> assets = State::getAssets(state, addressStr);

          std::vector<crow::json::wvalue> assetList;
          assetList.reserve(assets.size());
          for (const auto &assetId : assets) {
            assetList.push_back(assetId);
          }

          crow::json::wvalue res;
          res["address"] = addressStr;
          res["balance"] = State::getBalance(state, addressStr);
          res["assets"] = std::move(assetList);
          res["recentTransactions"] = collectTransactionsForAddress(
              chainSnapshot, addressStr, kDefaultRecentTxCount);
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

  CROW_ROUTE(app_, "/transact/signed")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        nlohmann::json body;
        try {
          body = nlohmann::json::parse(req.body);
        } catch (const std::exception &) {
          return crow::response(400, "Invalid JSON body");
        }

        if (!body.contains("secretKey") || !body["secretKey"].is_string()) {
          return crow::response(400, "Missing or invalid secretKey");
        }

        EVP_PKEY *keyPair = OpenSSLWrapper::keyPairFromPrivateHex(
            body["secretKey"].get<std::string>());
        if (!keyPair) {
          return crow::response(400, "Invalid secretKey");
        }

        try {
          const std::string fromAddress = deriveAddressPem(keyPair);
          if (fromAddress.empty()) {
            EVP_PKEY_free(keyPair);
            return crow::response(500, "Failed to derive signer address");
          }

          body.erase("secretKey");
          body["from"] = fromAddress;
          body["signature"] = "";
          if (!body.contains("id") ||
              (body["id"].is_string() && body["id"].get<std::string>().empty())) {
            body["id"] = generateUUID();
          }

          auto txn = TxnFactory::createTxn(body);
          const auto sigBytes =
              OpenSSLWrapper::sign(keyPair, txn->toSignableJson().dump());
          EVP_PKEY_free(keyPair);

          if (sigBytes.empty()) {
            return crow::response(500, "Failed to sign transaction");
          }
          txn->signature = HexUtils::toHex(sigBytes);

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
          EVP_PKEY_free(keyPair);
          return crow::response(400, e.what());
        }
      });

  CROW_ROUTE(app_, "/mine")
      .methods(crow::HTTPMethod::POST)([this](const crow::request &req) {
        try {
          std::string minerAddress;
          if (!req.body.empty()) {
            nlohmann::json body;
            try {
              body = nlohmann::json::parse(req.body);
            } catch (const std::exception &) {
              return crow::response(400, "Invalid JSON body");
            }

            if (body.contains("minerAddress")) {
              if (!body["minerAddress"].is_string()) {
                return crow::response(400, "minerAddress must be a string");
              }
              minerAddress = body["minerAddress"].get<std::string>();
            }
          }

          Block newBlock = miner_.mine(minerAddress);
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
