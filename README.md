# Station de Supervision IoT — Nuclear Reactor

Projet ESP32 PlatformIO simulant une station locale de supervision critique : acquisition capteurs, contrôle autonome, alarme, interface web embarquée, publication MQTT QoS 1 et stockage offline LittleFS.

## Architecture

- `src/sensors/` : `HW-486`, `DS18B20`, `DHT22`, encodeur `HW-040`.
- `src/actuators/` : servo `SG90`, 3 LEDs individuelles, buzzer.
- `src/network/` : WiFi et MQTT authentifiable.
- `src/storage/` : file JSON offline dans LittleFS.
- `src/web/` : serveur HTTP local et API protégée.
- `data/` : interface web HTML/CSS/JS servie par LittleFS.
- `docs/architecture.md` : diagramme logique, tâches FreeRTOS et flux réseau.

## Brochage par défaut

| Composant | Pin ESP32 |
| --- | --- |
| HW-486 analogique | GPIO 34 |
| DS18B20 OneWire | GPIO 4 |
| DHT22 OUT | GPIO 21 |
| Encodeur CLK / DT / SW | GPIO 18 / 19 / 5 |
| Servo SG90 | GPIO 13 |
| LEDs Rouge / Verte / Bleue | GPIO 25 / 26 / 27 |
| Buzzer HW-508 | GPIO 14 |

Notes de câblage :

- DS18B20 : résistance pull-up `4.7kΩ` obligatoire entre `DATA` et `3.3V`.
- DHT22 : `+` sur `3V3`, `-` sur `GND`, `OUT` sur `GPIO21`.
- HW-486 : `GPIO34` est une entrée ADC uniquement.
- SG90 : alimentation `5V`, masse commune avec l’ESP32.
- LEDs individuelles : résistance `220Ω` entre chaque GPIO et la patte longue, pattes courtes au `GND`.
- HW-508 : piloté en `HIGH` / `LOW` sur `GPIO14` pour buzzer actif.

## Configuration

1. Modifier `include/AppConfig.h` pour renseigner `WifiSsid`, `WifiPassword`, `GroupId` et `DeviceId`.
2. Le MQTT est initialisé dans `/config.json` sur LittleFS et peut être modifié via l’interface web.
3. L’API locale utilise Basic Auth : `operator` / `reactor`.

## Format JSON telemetry

```json
{
  "schema": "nuclear-reactor.telemetry.v1",
  "deviceId": "reactor-esp32-01",
  "group": "groupe-nuclear",
  "timestampMs": 123456,
  "reactor": {
    "online": true,
    "status": "NOMINAL",
    "rodPositionPercent": 42,
    "emergencyStop": false,
    "alarmAcknowledged": false
  },
  "measurements": {
    "lightRaw": 2200,
    "coreRadiationC": 36.5,
    "roomTemperatureC": 24.1,
    "roomHumidityPercent": 48.2
  },
  "system": {
    "wifiConnected": true,
    "mqttConnected": true,
    "freeHeap": 180000,
    "uptimeMs": 123456,
    "offlineQueueSize": 0
  }
}
```

Topic publié : `campus/<groupe>/<deviceID>/data`.

Topic commandes : `campus/<groupe>/<deviceID>/cmd`.

Commandes acceptées :

```json
{"command":"ACK_ALARM"}
{"command":"EMERGENCY_STOP"}
{"command":"RESET_EMERGENCY"}
{"command":"SET_ROD","value":75}
```

## Commandes PlatformIO

```powershell
pio run
pio run --target upload
pio run --target uploadfs
pio device monitor
```

## Résilience offline

Si MQTT est indisponible, chaque trame telemetry est ajoutée à `/offline.log` en JSON Lines. Au retour de la connexion, les trames sont retransmises automatiquement avant la nouvelle mesure.

## Ordonnancement FreeRTOS

- Capteurs : lecture toutes les 2 secondes.
- MQTT : publication toutes les 10 secondes.
- Bouton d’urgence : surveillance toutes les 50 ms via la tâche de contrôle.
- Pas de `delay()` bloquant : chaque tâche utilise `vTaskDelayUntil`.

## Vérifier que ça fonctionne

1. Flasher le système de fichiers puis le firmware :

```powershell
pio run --target uploadfs --upload-port COM5
pio run --target upload --upload-port COM5
```

2. Ouvrir le moniteur série :

```powershell
pio device monitor --port COM5 --baud 115200
```

3. Vérifier les logs toutes les 5 secondes :

```text
[reactor] status=NOMINAL online=yes rod=42% core=24.5C room=23.8C humidity=48.2% light=2300 wifi=ok mqtt=ok ip=192.168.1.42 heap=180000 offline=0
```

4. Ouvrir l’interface web avec l’adresse `ip=` affichée dans les logs :

```text
http://192.168.1.42/
```

5. Tests physiques rapides :

- Éclairer / cacher le `HW-486` doit changer `online` et `light`.
- Chauffer doucement le `DS18B20` doit faire monter `core`.
- Le `DHT22` doit afficher `room` et `humidity`.
- Tourner le `HW-040` doit changer `rod` et bouger le servo.
- Appui court sur `SW` acquitte l’alarme.
- Appui long sur `SW` déclenche `EMERGENCY_STOP`.
- Débrancher le WiFi/MQTT doit augmenter `offline`, puis revenir à `0` après reconnexion.
