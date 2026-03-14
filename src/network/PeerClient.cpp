#include "PeerClient.h"
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>

namespace {
constexpr auto kSyncInterval = std::chrono::milliseconds(1000);
} // namespace

PeerClient::PeerClient(MessageHandler handler) : onMessage_(std::move(handler)) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

PeerClient::~PeerClient() {
  {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (const auto &connection : connections_) {
      connection->running = false;
    }
  }

  for (const auto &connection : connections_) {
    if (connection->syncThread.joinable()) {
      connection->syncThread.join();
    }
  }
  connections_.clear();
  curl_global_cleanup();
}

bool PeerClient::connectToPeer(const std::string &url) {
  auto connection = std::make_shared<Connection>();
  connection->url = url;
  connection->baseHttpUrl = normalizeBaseUrl(url);
  const std::string healthUrl = connection->baseHttpUrl + "/health";

  if (httpGet(healthUrl).empty()) {
    std::cerr << "[PeerClient] Failed to connect " << url << "\n";
    return false;
  }

  connection->running = true;
  connection->syncThread = std::thread(&PeerClient::syncLoop, this, connection);

  std::lock_guard<std::mutex> lock(connectionsMutex_);
  connections_.push_back(connection);
  std::cout << "[PeerClient] Connected to " << url << " via HTTP sync\n";
  return true;
}

void PeerClient::broadcast(const std::string &message) {
  std::vector<std::shared_ptr<Connection>> snapshot;
  {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    snapshot = connections_;
  }

  for (const auto &connection : snapshot) {
    if (!connection->running) {
      continue;
    }
    const std::string postUrl = connection->baseHttpUrl + "/peer/message";
    if (!httpPostJson(postUrl, message)) {
      std::cerr << "[PeerClient] Send failed to " << connection->url << "\n";
    }
  }
}

void PeerClient::syncLoop(const std::shared_ptr<Connection> &connection) {
  std::string lastBlocksPayload;
  while (connection->running) {
    const std::string blocksUrl = connection->baseHttpUrl + "/blocks";
    const std::string blocksPayload = httpGet(blocksUrl);
    if (!blocksPayload.empty() && blocksPayload != lastBlocksPayload) {
      try {
        nlohmann::json parsed = nlohmann::json::parse(blocksPayload);
        if (parsed.contains("blocks")) {
          nlohmann::json msg{
              {"type", "CHAIN"},
              {"data", parsed["blocks"]},
          };
          if (onMessage_) {
            onMessage_(msg.dump());
          }
          lastBlocksPayload = blocksPayload;
        }
      } catch (const std::exception &) {
        // ignore parse failures in sync loop
      }
    }
    std::this_thread::sleep_for(kSyncInterval);
  }
}

std::string PeerClient::normalizeBaseUrl(const std::string &peerUrl) {
  std::string url = peerUrl;
  if (url.rfind("ws://", 0) == 0) {
    url = "http://" + url.substr(5);
  } else if (url.rfind("wss://", 0) == 0) {
    url = "https://" + url.substr(6);
  }

  if (url.size() >= 3 && url.substr(url.size() - 3) == "/ws") {
    url = url.substr(0, url.size() - 3);
  }
  if (!url.empty() && url.back() == '/') {
    url.pop_back();
  }
  return url;
}

size_t PeerClient::writeCallback(void *contents, size_t size, size_t nmemb,
                                 void *userp) {
  size_t totalSize = size * nmemb;
  auto *output = static_cast<std::string *>(userp);
  output->append(static_cast<char *>(contents), totalSize);
  return totalSize;
}

std::string PeerClient::httpGet(const std::string &url) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return "";
  }

  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1500L);

  CURLcode result = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
  curl_easy_cleanup(curl);

  if (result != CURLE_OK || status < 200 || status >= 300) {
    return "";
  }
  return response;
}

bool PeerClient::httpPostJson(const std::string &url, const std::string &body) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    return false;
  }

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 2000L);

  CURLcode result = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (result != CURLE_OK) {
    return false;
  }
  return status >= 200 && status < 300;
}
