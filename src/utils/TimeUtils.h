#pragma once
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
inline int64_t getCurrentTime() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

inline std::string getFormattedTimestamp(int64_t ms_since_epoch) {
  using namespace std::chrono;
  system_clock::time_point tp{milliseconds(ms_since_epoch)};

  std::time_t tt = system_clock::to_time_t(tp);

  std::tm tm{};
#ifdef _WIN32 // for function safety in Windows/POSIX systems
  localtime_s(&tm, &tt);
#else
  localtime_r(&tt, &tm);
#endif

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << '.' << std::setw(3)
      << std::setfill('0') << (ms_since_epoch % 1000);

  return oss.str();
}
