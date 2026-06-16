#pragma once

#include <Arduino.h>
#include <functional>

class OfflineStore {
 public:
  bool begin();
  bool append(const String& jsonLine);
  size_t count();
  bool replay(const std::function<bool(const String&)>& publisher);

 private:
  const char* path_ = "/offline.log";
  const char* tempPath_ = "/offline.tmp";
};
