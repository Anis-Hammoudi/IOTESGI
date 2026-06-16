# Rapport Technique : Système IoT Sécurisé et Autonome

**Projet de fin de module : Conception d'une station IoT de supervision de réacteur**
**Équipe :** Mehdi CHEDAD, Anis Hammoudi, Sanaa Zouine

---

## 1. Introduction et Contexte

Dans le cadre du déploiement de stations IoT pour la surveillance de bâtiments techniques (ici simulé par un réacteur nucléaire), ce rapport détaille la conception du firmware et de l'infrastructure logicielle d'une station autonome et sécurisée à base d'ESP32. 

L'objectif principal est de garantir la fiabilité de l'acquisition des capteurs, de survivre aux pannes de réseau (mode hors-ligne) et de maintenir un système de contrôle local et distant hautement réactif. Le projet respecte scrupuleusement le cahier des charges : architecture modulaire, système multitâche (FreeRTOS) sans code monolithique ni boucles d'attente actives (`delay()`).

---

## 2. Architecture Matérielle

La station utilise les capteurs et actionneurs suivants pour répondre aux contraintes minimales et de supervision avancée :

### Capteurs (Acquisition)
*   **DHT22 (Environnement)** : Mesure de la température et de l'humidité ambiante de la salle de contrôle.
*   **DS18B20 (Avancé)** : Mesure précise de la température du cœur du réacteur (capteur étanche OneWire).
*   **Photorésistance HW-486 (Lumière)** : Détection d'état ou d'ouverture (détermine si le réacteur est "en ligne").
*   **Encodeur rotatif + Bouton poussoir (Interaction)** : Contrôle manuel du pourcentage d'insertion des barres de contrôle et acquittement des alarmes (ou arrêt d'urgence par appui long).

### Actionneurs (Contrôle)
*   **Servo SG90 (Intelligent)** : Simule l'insertion mécanique des barres de contrôle du réacteur (0 à 100%).
*   **Buzzer HW-508 (Simple)** : Alarme sonore en cas de criticité (températures anormales).
*   **LED RGB NeoPixel / StatusLed** : Indicateur visuel d'état (Vert = Nominal, Orange = Avertissement, Rouge = Critique, Bleu = Perte MQTT).

---

## 3. Architecture Logicielle Modulaire

Afin d'éviter l'écueil du code monolithique (logique métier dans `loop()`), le code source a été strictement séparé en dossiers fonctionnels :

*   `src/sensors/` : Classes d'abstraction des capteurs (`EnvironmentSensor`, `RadiationSensor`, etc.) intégrant le filtrage logiciel.
*   `src/actuators/` : Classes de pilotage du matériel (`ControlRodServo`, `AlarmBuzzer`, `StatusLed`).
*   `src/network/` : Gestionnaires de connexion `ConnectivityManager` (WiFi) et `MqttClientManager` (MQTT).
*   `src/storage/` : Système de fichiers LittleFS pour la file d'attente hors-ligne (`OfflineStore`).
*   `src/web/` : Serveur web asynchrone embarqué (`WebServerManager`).
*   `src/models/` : Structures de données (ex. `ReactorState`) garantissant la cohérence de la mémoire.

---

## 4. Fonctionnalités Clés et Badges Débloqués

### 🟢 Sensor Engineer : Acquisition fiable et filtrage
Les mesures des capteurs ne sont jamais remontées brutes si elles sont erratiques.
*   **Rejet d'aberrations** : Le `EnvironmentSensor` (DHT22) ignore les valeurs impossibles (ex. humidité > 100% ou hors spectre de température).
*   **Filtrage EMA (Exponential Moving Average)** : Les températures sont lissées avec un coefficient alpha pour éviter les pics soudains et les fausses alarmes matérielles.
*   **Horodatage (Timestamp)** : Toutes les données sont horodatées localement dès la lecture.

### 🔵 Network Engineer : Communication MQTT Robuste
L'ESP32 publie sur `campus/<groupe>/<deviceID>/data` et écoute les commandes sur `/cmd`.
*   **Qualité de service (QoS 1)** : Garantit qu'au moins une livraison de la télémétrie est confirmée par le broker.
*   **Reconnexion asynchrone** : En cas de perte WiFi ou MQTT, le système tente de se reconnecter périodiquement toutes les 5 secondes sans jamais bloquer l'acquisition des capteurs ou la boucle de sécurité.

### ⚫ Reliability Engineer : Survie aux pannes (Mode Offline)
Si le réseau MQTT tombe, la station IoT continue de fonctionner :
*   **Stockage JSON** : Les trames télémétriques sont écrites dans la mémoire flash (`LittleFS`) au format strict JSON requis.
*   **Retransmission automatique** : Dès le rétablissement de la connexion, `OfflineStore` dépile les anciens JSON et les re-publie avant d'envoyer les nouvelles données.

### 🔴 Security Engineer : Sécurité minimale
*   **Authentification MQTT** : Utilisation d'identifiants (nom d'utilisateur et mot de passe).
*   **Protection API Locale** : L'interface web et les endpoints API (Configuration, Commandes) nécessitent une authentification HTTP *Basic Auth* (User: `operator`).
*   **Validation JSON** : Toute commande reçue via l'interface web ou MQTT est passée par `deserializeJson()` avec vérification d'erreurs avant application (`validateAndApplyCommand`), évitant les crashs mémoire (Buffer Overflows).

### 🟣 Full-Stack IoT : Interface Web & Node-RED
*   **Web embarqué** : Fichiers déportés `index.html`, `app.js`, `style.css` stockés dans `/data` et servis par l'ESP32. Permet la visualisation en direct (AJAX), la configuration MQTT et le pilotage des actionneurs.
*   **Node-RED & Grafana** : Un flux externe Node-RED (`node_red_flow.json` fourni) reçoit le MQTT, intègre la donnée en base NoSQL (InfluxDB) et permet un affichage complet sur **Grafana** (dashboard des mesures et surveillance de la robustesse).

### 🟡 Performance Engineer : Optimisation mémoire
*   **Supervision** : Une tâche `supervisorTask` tourne en fond et affiche périodiquement le `freeHeap` (RAM libre), l'uptime et la taille de la file hors-ligne. L'utilisation d'objets alloués dynamiquement est limitée pour éviter la fragmentation mémoire.

---

## 5. Gestion Multitâche & Analyse Temps Réel (FreeRTOS)

Le cœur de notre architecture repose sur l'ordonnanceur FreeRTOS. Le choix du RTOS face à une boucle `loop()` séquentielle est dicté par le besoin de **déterminisme et de réactivité critique**.

### Affectation des Priorités (Rate Monotonic Scheduling)
Les priorités (de 1 à 4) reflètent la criticité :
*   **Priorité 4 (controlTask)** : Pilotage de l'encodeur, bouton d'urgence et servo. C'est la tâche de sécurité critique, elle préempte toutes les autres pour réagir en < 50ms.
*   **Priorité 3 (acquisitionTask)** : Lecture capteurs. Ne doit pas être bloquée par le réseau.
*   **Priorité 2 (networkTask)** : WiFi, MQTT. Tolère des délais, s'exécute quand le matériel est prêt.
*   **Priorité 1 (webTask, supervisorTask)** : Tâches de confort (statistiques, Web UI) utilisant le temps CPU restant.

---

## 6. Réponses aux Questions Subsidiaires

**1. Rôle du scheduler, tâche vs fonction, intérêt de vTaskDelay(), pourquoi l'architecture multitâche ?**
Le *scheduler* orchestre l'exécution en allouant le CPU aux tâches selon leur priorité. Une *fonction* s'exécute de façon séquentielle de la première à la dernière ligne puis s'arrête, alors qu'une *tâche* FreeRTOS est une boucle infinie dotée de sa propre pile, préemptable à tout moment. `vTaskDelay()` suspend une tâche de façon non-bloquante, libérant le CPU. L'architecture multitâche est cruciale ici pour réagir instantanément aux alarmes (bouton urgence) sans être bloqué par des latences réseau (typiques d'une simple `loop()`).

**2. Que signifie vTaskDelay(pdMS_TO_TICKS(1000)); ? Étapes et impact sur les autres tâches ?**
Cette instruction bloque la tâche courante pendant 1000 millisecondes (converties en ticks matériels). 
*Étapes* : La tâche passe de *Running* à *Blocked*. Le scheduler cède immédiatement le CPU à la tâche *Ready* la plus prioritaire. Un timer matériel compte les ticks en fond. Après 1000ms, la tâche repasse à l'état *Ready* et peut reprendre le CPU. Pendant ce temps de pause, *les autres tâches* utilisent librement le CPU.

**3. Justifier les priorités FreeRTOS.**
Justifiées dans la Section 5 ci-dessus : la sécurité mécanique (Encodeur/Servo/Bouton urgence) préempte tout (`Prio 4`), suivie de la lecture capteur pour ne perdre aucune alerte (`Prio 3`). Le réseau (`Prio 2`) et l'interface Web (`Prio 1`) s'exécutent en arrière-plan sans jamais figer la sécurité.

**4. Pourquoi faut-il presque toujours un vTaskDelay(...) ?**
Pour empêcher une tâche de s'accaparer 100% du CPU (causant la *starvation* des tâches inférieures) et pour permettre au Watchdog matériel (WDT) de se réinitialiser. Sans délai, l'ESP32 plantera.

**5. Dans `while(true) { mqtt.publish(...); }` (sans vTaskDelay) :**
Le code inonderait le broker MQTT, bloquerait instantanément toutes les tâches de priorités inférieures ou égales, et déclencherait presque immédiatement un redémarrage d'urgence (Crash) provoqué par le Watchdog Timer de l'ESP32.

**6. `TaskMQTT` priorité 20 vs `TaskSensors` priorité 1 : Système meilleur ?**
Non, le système serait catastrophique. Le réseau TCP/MQTT implique de grandes latences (timeouts). Avec une priorité de 20, le réseau préemptera l'acquisition de données. La moindre perte de paquet WiFi empêcherait la lecture des capteurs, rendant la station aveugle à toute explosion thermique du réacteur.

**7. Variable globale partagée `temperature` : le code est-il correct ?**
Non, ce code n'est pas "Thread-Safe" (risque de *Data Race*). Si les lectures/écritures ne sont pas atomiques, ou que les cœurs accèdent à la RAM simultanément (sur ESP32 dual-core), la tâche web peut lire une valeur corrompue (à moitié modifiée par `TaskSensors`).

**8. Mutex, Queue ou Sémaphore (définition et Mutex vs Queue) :**
*   **Mutex** : Verrou assurant l'exclusion mutuelle pour protéger une ressource critique.
*   **Queue** : File d'attente FIFO thread-safe.
*   **Sémaphore** : Compteur ou simple signalisation entre tâches.
On utilise un *Mutex* pour protéger l'état instantané (ex: `ReactorState`), où l'on veut juste la "dernière valeur", alors qu'une *Queue* est utilisée pour transférer un flux d'événements à dépiler (ex: trames MQTT à envoyer) sans perdre l'historique.

**9. `TaskSensors { temp = read(); }` -> `TaskMQTT { publish(temp); }` : Pourquoi une Queue est meilleure ?**
Avec une Queue, l'acquisition et le réseau sont totalement asynchrones. La `TaskSensors` pousse la valeur et repart faire son travail en 1 milliseconde. La `TaskMQTT` dépile à son rythme. Si MQTT ralentit à cause d'un réseau instable, `TaskSensors` ne sera jamais bloquée en attente.

**10. Que signifie Starvation ? Exemple.**
La "Famine" ou *Starvation* survient quand des tâches de priorité élevée consomment tout le temps CPU, laissant les tâches de faible priorité dans l'état *Ready* de façon perpétuelle, sans jamais s'exécuter. Exemple : Si une tâche de cryptographie (`Prio 3`) tourne en boucle sans `vTaskDelay`, une tâche d'affichage LED (`Prio 1`) ne clignotera jamais.

**11. Pourquoi FreeRTOS plutôt qu'une boucle Arduino ?**
Une boucle classique (`void loop()`) exécute le code séquentiellement. Si `mqtt.connect()` tente de se connecter au WiFi et bloque pendant 5 secondes (timeout réseau standard), le code est complètement gelé pendant 5 secondes : le bouton d'arrêt d'urgence est mort, et les capteurs ne mesurent plus rien. FreeRTOS (grâce au scheduler préemptif) garantit que la lecture du bouton et des capteurs interrompra instantanément la fonction réseau défaillante.

---

## 7. Conclusion
Le système livré correspond intégralement aux spécifications demandées pour une architecture IoT industrielle. L'appareil est modulaire, autonome, résistant aux coupures réseau (spooling local) et sécurisé, tout en garantissant des temps de réaction déterministes.
