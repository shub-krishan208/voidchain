#pragma once

#include <atomic>
#include <curl/curl.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class PeerClient {
public:
  using MessageHandler = std::function<void(const std::string &)>;

  explicit PeerClient(MessageHandler handler);
  ~PeerClient();

  bool connectToPeer(const std::string &url);
  void broadcast(const std::string &message);

private:
  struct Connection {
    std::string url;
    std::string baseHttpUrl;
    std::thread syncThread;
    std::atomic<bool> running{false};
  };

  void syncLoop(const std::shared_ptr<Connection> &connection);
  static std::string normalizeBaseUrl(const std::string &peerUrl);
  static size_t writeCallback(void *contents, size_t size, size_t nmemb,
                              void *userp);
  static std::string httpGet(const std::string &url);
  static bool httpPostJson(const std::string &url, const std::string &body);

  MessageHandler onMessage_;
  std::vector<std::shared_ptr<Connection>> connections_;
  std::mutex connectionsMutex_;
};
