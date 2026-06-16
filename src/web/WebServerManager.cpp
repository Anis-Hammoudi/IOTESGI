#include "WebServerManager.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {
constexpr const char* WebUser = "operator";
constexpr const char* WebPassword = "reactor";
}

WebServerManager::WebServerManager() : server_(80) {}

void WebServerManager::begin(StateProvider stateProvider,
                             TelemetryProvider telemetryProvider,
                             ConfigSaver configSaver,
                             CommandHandler commandHandler) {
  stateProvider_ = stateProvider;
  telemetryProvider_ = telemetryProvider;
  configSaver_ = configSaver;
  commandHandler_ = commandHandler;
  setupRoutes();
  server_.begin();
}

void WebServerManager::loop() {
  server_.handleClient();
}

bool WebServerManager::authenticated() {
  if (server_.authenticate(WebUser, WebPassword)) {
    return true;
  }
  server_.requestAuthentication();
  return false;
}

void WebServerManager::sendJson(int code, const String& body) {
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(code, "application/json", body);
}

void WebServerManager::setupRoutes() {
  server_.on("/", HTTP_GET, [this]() {
    File file = LittleFS.open("/index.html", FILE_READ);
    if (!file) {
      server_.send(404, "text/plain", "index.html missing");
      return;
    }
    server_.streamFile(file, "text/html");
    file.close();
  });

  server_.serveStatic("/style.css", LittleFS, "/style.css", "max-age=86400");
  server_.serveStatic("/app.js", LittleFS, "/app.js", "max-age=86400");

  server_.on("/api/state", HTTP_GET, [this]() {
    sendJson(200, telemetryProvider_ ? telemetryProvider_() : "{}");
  });

  server_.on("/api/config/mqtt", HTTP_POST, [this]() {
    if (!authenticated()) {
      return;
    }

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, server_.arg("plain"));
    if (error) {
      sendJson(400, "{\"error\":\"invalid_json\"}");
      return;
    }

    MqttSettings settings;
    settings.host = doc["host"] | "";
    settings.port = doc["port"] | 1883;
    settings.username = doc["username"] | "";
    settings.password = doc["password"] | "";
    settings.baseTopic = doc["baseTopic"] | "";

    if (settings.host.length() == 0 || settings.baseTopic.length() == 0) {
      sendJson(422, "{\"error\":\"missing_host_or_baseTopic\"}");
      return;
    }

    const bool saved = configSaver_ && configSaver_(settings);
    sendJson(saved ? 200 : 500, saved ? "{\"ok\":true}" : "{\"error\":\"save_failed\"}");
  });

  server_.on("/api/command", HTTP_POST, [this]() {
    if (!authenticated()) {
      return;
    }

    const String body = server_.arg("plain");
    DynamicJsonDocument doc(256);
    if (deserializeJson(doc, body)) {
      sendJson(400, "{\"error\":\"invalid_json\"}");
      return;
    }

    const char* command = doc["command"] | "";
    if (String(command) != "ACK_ALARM" &&
        String(command) != "EMERGENCY_STOP" &&
        String(command) != "RESET_EMERGENCY" &&
        String(command) != "SET_ROD") {
      sendJson(422, "{\"error\":\"invalid_command\"}");
      return;
    }

    if (commandHandler_) {
      commandHandler_(body);
    }
    sendJson(200, "{\"ok\":true}");
  });

  server_.onNotFound([this]() {
    sendJson(404, "{\"error\":\"not_found\"}");
  });
}
