#pragma once

#include <Arduino.h>

#include "models/ReactorState.h"

String buildTelemetryJson(const ReactorState& state);
bool validateAndApplyCommand(const String& payload, ReactorState& state);
