# Station de Supervision IoT — Nuclear Reactor

**Projet de fin de module : Système IoT sécurisé et autonome**
**Groupe :** Mehdi CHEDAD, Anis Hammoudi, Sanaa Zouine

Projet ESP32 PlatformIO simulant une station locale de supervision critique : acquisition capteurs, contrôle autonome, alarme, interface web embarquée, publication MQTT QoS 1 et stockage offline LittleFS.

## 🎖️ Badges Débloqués

*   🟢 **Sensor Engineer** : Acquisition fiable avec filtrage EMA et rejet des valeurs aberrantes.
*   🔵 **Network Engineer** : MQTT robuste avec reconnexion automatique et QoS 1.
*   🟠 **Embedded Architect** : Architecture multitâche propre (FreeRTOS) et code modulaire.
*   🔴 **Security Engineer** : Validation stricte des JSON entrants et authentification (MQTT + Web API).
*   🟣 **Full-Stack IoT** : Interface Web embarquée complète et flux Node-RED.
*   ⚫ **Reliability Engineer** : Survie aux pannes réseau avec stockage offline local (LittleFS) et spooling.
*   🟡 **Performance Engineer** : Optimisation mémoire et supervision des ressources (Heap/Uptime).
*   🏆 **Bonus (Grafana)** : Flux Node-RED prévu pour l'historisation en base temporelle (InfluxDB) et affichage sur Grafana.

## Architecture Logique

- `src/sensors/` : `HW-486`, `DS18B20`, `DHT22`, encodeur `HW-040`.
- `src/actuators/` : servo `SG90`, LEDs RVB individuelles, buzzer.
- `src/network/` : WiFi et MQTT authentifiable.
- `src/storage/` : file JSON offline dans LittleFS.
- `src/web/` : serveur HTTP local et API protégée.
- `data/` : interface web HTML/CSS/JS servie par LittleFS.

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

## Format JSON Telemetry (Imposé)

```json
{
 "device": "ESP32-01",
 "ts": 123456789,
 "data": {
   "temp": 24.1,
   "humidity": 48.2,
   "coreRadiationC": 36.5,
   "lightRaw": 2200,
   "status": "NOMINAL",
   "rodPositionPercent": 42,
   "emergencyStop": false,
   "online": true,
   "alarmAcknowledged": false,
   "wifiConnected": true,
   "mqttConnected": true,
   "freeHeap": 180000,
   "uptimeMs": 123456,
   "offlineQueueSize": 0
 }
}
```

Topic publié : `campus/<groupe>/<deviceID>/data`  
Topic commandes : `campus/<groupe>/<deviceID>/cmd`

Commandes MQTT / API acceptées :
```json
{"command":"ACK_ALARM"}
{"command":"EMERGENCY_STOP"}
{"command":"RESET_EMERGENCY"}
{"command":"SET_ROD","value":75}
```

## Commandes PlatformIO

Pour nettoyer le projet (ce qui efface les anciens builds) :
```powershell
pio run -t clean
```

Pour compiler et flasher l'ESP32 :
```powershell
pio run --target uploadfs --upload-port COM5
pio run --target upload --upload-port COM5
```

Pour monitorer les logs de l'ESP32 :
```powershell
pio device monitor --port COM5 --baud 115200
```

## Vérifier que ça fonctionne

1. Flasher le système de fichiers (`uploadfs`) puis le firmware (`upload`).
2. Observer le moniteur série. Le système doit afficher un diagnostic toutes les 5 secondes indiquant l'état nominal ou critique, ainsi que les mesures environnementales.
3. Se connecter à l'interface Web (sur l'IP locale affichée dans le moniteur série) en utilisant les identifiants : `operator` / `reactor`.
4. Effectuer des tests physiques :
   - Modifier la position de l'encodeur pour bouger le servo (barres de contrôle).
   - Simuler une surchauffe du capteur DS18B20 pour déclencher l'alarme (Buzzer + LED Rouge).
   - Un appui court sur l'encodeur doit acquitter l'alarme. Un appui long active l'arrêt d'urgence.
   - Couper le réseau WiFi pour observer la file hors-ligne (Offline Queue) grandir, puis se vider une fois la connexion rétablie.
