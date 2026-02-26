#pragma once
#include <cstddef>
#include <iomanip>
#include <openssl/core_dispatch.h>
#include <sstream>
#include <string>
#include <vector>

namespace HexUtils {
inline std::string toHex(const std::vector<unsigned char> &data) {
  std::basic_ostringstream<char> oss;
  for (auto byte : data)
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(byte);
  return oss.str();
}

inline std::vector<unsigned char> fromHex(const std::string &hex) {
  std::vector<unsigned char> bytes;
  for (size_t i = 0; i < hex.size(); i += 2) {
    std::string byteString = hex.substr(i, 2);
    unsigned char byte =
        static_cast<unsigned char>(std::stoi(byteString, nullptr, 16));
    bytes.push_back(byte);
  }
  return bytes;
}
} // namespace HexUtils