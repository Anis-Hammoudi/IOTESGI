#include "OfflineStore.h"

#include <LittleFS.h>

bool OfflineStore::begin() {
  if (!LittleFS.exists(path_)) {
    File f = LittleFS.open(path_, FILE_WRITE);
    if (f) f.close();
  }
  return true;
}

bool OfflineStore::append(const String& jsonLine) {
  String allData = "";
  File r = LittleFS.open(path_, FILE_READ);
  if (r) {
    allData = r.readString();
    r.close();
  }
  
  File w = LittleFS.open(path_, FILE_WRITE);
  if (!w) return false;
  if (allData.length() > 0) w.print(allData);
  w.println(jsonLine);
  w.close();
  return true;
}

size_t OfflineStore::count() {
  File file = LittleFS.open(path_, FILE_READ);
  if (!file) return 0;

  size_t lines = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) ++lines;
  }
  file.close();
  return lines;
}

bool OfflineStore::replay(const std::function<bool(const String&)>& publisher) {
  File input = LittleFS.open(path_, FILE_READ);
  if (!input) return true;

  File output = LittleFS.open(tempPath_, FILE_WRITE);
  if (!output) {
    input.close();
    return false;
  }

  bool blocked = false;
  while (input.available()) {
    String line = input.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    
    if (!blocked && publisher(line)) {
      continue;
    }
    blocked = true;
    output.println(line);
  }

  input.close();
  output.close();

  if (blocked) {
    LittleFS.remove(path_);
    LittleFS.rename(tempPath_, path_);
  } else {
    LittleFS.remove(tempPath_);
    File clearFile = LittleFS.open(path_, FILE_WRITE);
    if (clearFile) clearFile.close();
  }
  return true;
}
