#include "../src/core/MerkleTree.h"
#include "../src/utils/hashing.h"
#include <curl/curl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  const size_t total = size * nmemb;
  auto *out = static_cast<std::string *>(userp);
  out->append(static_cast<char *>(contents), total);
  return total;
}

std::string httpGet(const std::string &url) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    throw std::runtime_error("Failed to initialize curl");
  }

  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 4000L);

  CURLcode result = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  curl_easy_cleanup(curl);

  if (result != CURLE_OK || status < 200 || status >= 300) {
    throw std::runtime_error("HTTP GET failed for: " + url);
  }

  return response;
}

std::string hashTxnJson(const nlohmann::json &txData) {
  Hasher h;
  h.add(txData.dump());
  return h.finish();
}

std::vector<MerkleTree::ProofNode>
parseProofNodes(const nlohmann::json &proofArray) {
  std::vector<MerkleTree::ProofNode> proof;
  for (const auto &node : proofArray) {
    proof.push_back(
        MerkleTree::ProofNode{node.at("hash"), node.at("isLeft").get<bool>()});
  }
  return proof;
}
} // namespace

int main(int argc, char **argv) {
  std::string nodeUrl = "http://localhost:18169";
  std::string txId;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--node" && i + 1 < argc) {
      nodeUrl = argv[++i];
    } else if (arg == "--txId" && i + 1 < argc) {
      txId = argv[++i];
    }
  }

  if (txId.empty()) {
    std::cerr << "Usage: citizen_client --txId <transaction-id> [--node "
                 "http://localhost:18169]\n";
    return 1;
  }

  if (!nodeUrl.empty() && nodeUrl.back() == '/') {
    nodeUrl.pop_back();
  }

  curl_global_init(CURL_GLOBAL_DEFAULT);
  try {
    const nlohmann::json headers =
        nlohmann::json::parse(httpGet(nodeUrl + "/headers"));
    if (!headers.contains("headers")) {
      throw std::runtime_error("Malformed /headers response");
    }

    const nlohmann::json proofPayload =
        nlohmann::json::parse(httpGet(nodeUrl + "/proof?txId=" + txId));

    const nlohmann::json txData = proofPayload.at("txData");
    const std::string root = proofPayload.at("root");
    const std::string txHash = hashTxnJson(txData);
    const std::vector<MerkleTree::ProofNode> proof =
        parseProofNodes(proofPayload.at("proof"));

    const bool merkleOk = MerkleTree::verifyProof(root, txHash, proof);
    if (!merkleOk) {
      std::cerr << "[FAIL] Merkle proof verification failed for txId=" << txId
                << "\n";
      curl_global_cleanup();
      return 2;
    }

    const std::string blockHash = proofPayload.at("block").at("hash");
    bool headerMatch = false;
    for (const auto &header : headers.at("headers")) {
      if (header.at("hash") == blockHash &&
          header.at("merkle_root").get<std::string>() == root) {
        headerMatch = true;
        break;
      }
    }
    if (!headerMatch) {
      std::cerr << "[FAIL] No header matched proof block/root for txId=" << txId
                << "\n";
      curl_global_cleanup();
      return 3;
    }

    std::cout << "[PASS] Proof verified for txId=" << txId << "\n";
    std::cout << "  node: " << nodeUrl << "\n";
    std::cout << "  block_hash: " << blockHash << "\n";
    std::cout << "  merkle_root: " << root << "\n";
    std::cout << "  tx_hash: " << txHash << "\n";
  } catch (const std::exception &e) {
    std::cerr << "[ERROR] " << e.what() << "\n";
    curl_global_cleanup();
    return 4;
  }

  curl_global_cleanup();
  return 0;
}
