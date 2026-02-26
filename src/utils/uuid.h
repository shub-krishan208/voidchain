#pragma once
#include <string>
#include <uuid/uuid.h>

inline std::string generateUUID() {
  uuid_t uuid;
  uuid_generate_random(uuid);

  char buffer[37]; // 36 chars + null
  uuid_unparse_lower(uuid, buffer);

  return std::string(buffer);
}