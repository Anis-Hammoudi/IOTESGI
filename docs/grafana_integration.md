# Intégration Grafana - Station IoT Nuclear Reactor

Ce document explique comment le code actuel de l'ESP32 répond aux exigences du badge **Bonus (Grafana)** et comment configurer l'infrastructure côté serveur pour exploiter ces données.

## 1. Respect des exigences par le code ESP32

Le code actuel de l'ESP32 est déjà **entièrement compatible** avec l'exigence Grafana. 
Il n'y a **aucune modification de code** à apporter sur l'ESP32.

L'ESP32 publie à intervalle régulier (toutes les 10 secondes par défaut) un flux JSON sur le topic MQTT `campus/<groupe>/<deviceID>/data`.

Ce flux contient toutes les informations requises par l'énoncé :

*   **Mesures capteurs** : `coreRadiationC` (DS18B20), `roomTemperatureC` (DHT22), `roomHumidityPercent` (DHT22), `lightRaw` (HW-486).
*   **État et Actionneurs** : `status` (NOMINAL, WARNING, CRITICAL, EMERGENCY), `rodPositionPercent` (Servo), `emergencyStop`.
*   **Métrique de robustesse** : L'ESP32 transmet `freeHeap` (RAM disponible), `uptimeMs` (temps depuis le démarrage), et `offlineQueueSize` (nombre de messages en attente d'envoi en cas de perte de connexion réseau).

**Exemple de payload envoyé par l'ESP32 :**
```json
{
  "device": "ESP32-X",
  "ts": 1234567,
  "data": {
    "temp": 24.1,
    "humidity": 48.2,
    "lightRaw": 2200,
    "coreRadiationC": 36.5,
    "status": "NOMINAL",
    "rodPositionPercent": 42,
    "freeHeap": 180000,
    "uptimeMs": 123456,
    "offlineQueueSize": 0
  }
}
```

## 2. Architecture de l'intégration côté serveur

Pour accomplir le bonus, vous devez mettre en place la chaîne de traitement suivante sur votre serveur (ou machine virtuelle) :

**ESP32**  --> `[ MQTT Broker ]` --> **Node-RED** --> `[ InfluxDB ]` --> **Grafana**

1.  **Node-RED** : Un flux Node-RED s'abonne au topic MQTT, parse le JSON reçu, formate les données et les insère dans InfluxDB.
2.  **InfluxDB** : C'est la *base temporelle compatible Grafana* demandée dans l'énoncé. Elle stocke l'historique des séries temporelles (time-series).
3.  **Grafana** : Se connecte à InfluxDB en tant que source de données (Data Source) pour afficher les graphes et générer des alertes.

## 3. Mise en place de Node-RED vers InfluxDB

Dans **Node-RED**, vous devez créer un flow avec les nœuds suivants :

1.  **Nœud MQTT in** : Écoute sur `campus/+/+/data`.
2.  **Nœud JSON** : Convertit la chaîne JSON en objet JavaScript (`msg.payload`).
3.  **Nœud Fonction** : Prépare la requête pour InfluxDB. Par exemple :
    ```javascript
    msg.payload = [
        {
            temperature: msg.payload.data.temp,
            humidity: msg.payload.data.humidity,
            core_radiation: msg.payload.data.coreRadiationC,
            rod_position: msg.payload.data.rodPositionPercent,
            free_heap: msg.payload.data.freeHeap,
            offline_queue: msg.payload.data.offlineQueueSize
        },
        {
            device: msg.payload.device
        }
    ];
    return msg;
    ```
4.  **Nœud InfluxDB out** : Enregistre le `msg.payload` formaté dans la base de données.

## 4. Création du Dashboard Grafana et Alertes

Une fois InfluxDB configuré comme source de données dans Grafana, vous devez créer un **Dashboard** avec différents panneaux (Panels) :

*   **Time series (Graphes) pour les capteurs** : Afficher `roomTemperatureC`, `roomHumidityPercent`, et `coreRadiationC` au fil du temps.
*   **Gauge / Bar gauge pour les actionneurs** : Afficher l'état d'insertion des barres de contrôle (`rodPositionPercent`).
*   **Stat / Time series pour la robustesse** : Un graphique montrant l'évolution du `freeHeap` (pour détecter les fuites mémoires) et le `offlineQueueSize` (pour détecter l'instabilité du réseau).

### Configuration de l'alerte

L'énoncé demande de "configurer une alerte pour détecter une absence de données ou une anomalie capteur".

Dans Grafana, sur le graphique de température par exemple :
1.  Allez dans l'onglet **Alerting** du panneau.
2.  Créez une règle d'alerte (Alert rule).
3.  **Condition d'anomalie** : Définissez une règle `WHEN max() OF query(A, 5m, now) IS ABOVE 50` (si la température dépasse 50°C pendant 5 minutes).
4.  **Condition d'absence de données (No Data)** : Dans la configuration de la règle d'alerte, trouvez la section **"No data and error handling"**. Configurez *"If no data or all values are null"* sur **Alerting**. Ainsi, si l'ESP32 s'éteint et arrête de publier pendant quelques minutes, Grafana déclenchera l'alerte.
5.  Configurez un canal de notification (Discord, Email, Telegram, etc.) pour recevoir l'alerte.
