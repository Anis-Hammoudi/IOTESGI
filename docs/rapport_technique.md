# Rapport Technique : Station IoT - Nuclear Reactor

Ce rapport accompagne le projet de fin de module "Système IoT sécurisé et autonome".
Le système implémente une supervision de réacteur (capteurs de lumière, DHT22, DS18B20) et des contrôles de sécurité avec une interface Web embarquée, du MQTT, et un mode hors-ligne.

---

## 1. Choix de l'Architecture et Rôle du Scheduler

Le choix de FreeRTOS pour ce projet plutôt qu'une boucle `loop()` séquentielle Arduino classique est justifié par le besoin de **déterminisme et de réactivité critique**.

*   **Rôle du scheduler** : Le scheduler (ou ordonnanceur) de FreeRTOS est le composant système qui décide à chaque "tick" quelle tâche doit être exécutée par le CPU. Il se base sur la priorité des tâches et leur état (Prêt, Bloqué, Suspendu).
*   **Tâche vs Fonction** : Une fonction standard s'exécute de la première à la dernière ligne puis rend la main. Une tâche FreeRTOS est un programme indépendant doté de sa propre pile d'exécution (stack) et d'une boucle infinie. Elle peut être interrompue (préemptée) à tout moment par le scheduler.
*   **vTaskDelay()** : Cette instruction met la tâche dans l'état *Bloqué* pendant un nombre donné de ticks. Contrairement à une boucle d'attente active ou un `delay()` basique, elle informe le scheduler que la tâche n'a plus besoin du CPU, permettant aux autres tâches de s'exécuter.
*   **Pourquoi le multitâche ici ?** : Le système doit réagir instantanément au bouton d'arrêt d'urgence. Si nous étions dans une boucle séquentielle et qu'un `mqtt.publish()` bloquait (par exemple à cause d'un réseau lent), l'arrêt d'urgence serait ignoré pendant ce laps de temps. Avec FreeRTOS, la tâche de contrôle interrompt le réseau pour agir.

## 2. Analyse de l'instruction `vTaskDelay`

Instruction : `vTaskDelay(pdMS_TO_TICKS(1000));`

*   **Signification** : Met en pause la tâche courante pour exactement 1000 millisecondes (converties en nombre de ticks selon la configuration de l'OS).
*   **Étapes** : 
    1. La tâche appelle la fonction.
    2. Son état passe de *Running* (en cours) à *Blocked* (bloqué).
    3. Le scheduler effectue un changement de contexte (context switch) et alloue le CPU à la tâche *Ready* de plus haute priorité.
    4. Un timer interne matériel décrémente le délai.
    5. Après 1000 ms, la tâche repasse à l'état *Ready* et le scheduler évalue à nouveau les priorités pour lui redonner le CPU si elle est la plus prioritaire.
*   **Les autres tâches** : Elles s'exécutent normalement et se partagent le CPU pendant que cette tâche "dort".

## 3. Justification des Priorités FreeRTOS

Les priorités ont été assignées (de 1 à 4, 4 étant la plus haute) en fonction de la criticité temporelle :

*   **Priorité 4 (Très Haute) - `controlTask`** : Gère l'encodeur, le bouton d'arrêt d'urgence et le pilotage du servo-moteur (barres de contrôle). C'est la tâche de sécurité critique, elle doit préempter toutes les autres tâches pour agir en moins de 50 ms.
*   **Priorité 3 (Haute) - `acquisitionTask`** : Lit les capteurs (I2C, OneWire). L'acquisition doit être régulière pour ne rater aucun dépassement de seuil, mais elle est moins critique qu'un appui d'urgence.
*   **Priorité 2 (Moyenne) - `networkTask`** : Gère la communication réseau (WiFi et MQTT). Peut tolérer des délais (la latence réseau est inhérente) et ne doit jamais bloquer les lectures de capteurs.
*   **Priorité 1 (Basse) - `webTask` et `supervisorTask`** : L'interface web et le log des statistiques mémoire sont des tâches de "confort". Elles utilisent le temps CPU restant sans jamais gêner les opérations vitales de la station.

## 4. Nécessité de `vTaskDelay()`

Il est vital de trouver au moins un `vTaskDelay(...)` (ou tout appel bloquant comme l'attente d'un Mutex ou d'une Queue) dans une boucle de tâche FreeRTOS. 
Sans cela, si la tâche est de forte priorité, elle consommera 100% du temps CPU. Le scheduler ne rendra jamais la main aux tâches de priorité inférieure (c'est la "famine" ou *starvation*). De plus, l'absence de pause empêche le Chien de Garde matériel (Watchdog Timer - WDT) d'être réinitialisé, ce qui fera planter l'ESP32.

## 5. Cas d'une boucle `while(true) { mqtt.publish(...); }`

Si cette fonction s'exécute sans aucun délai :
*   Le module va spammer le serveur MQTT.
*   Le CPU de l'ESP32 sera accaparé par cette tâche réseau.
*   Le Watchdog Timer va très rapidement détecter l'anomalie et redémarrer brutalement la carte.
*   Les autres tâches (si leur priorité est plus basse) subiront une "starvation".

## 6. Problème de l'inversion des priorités : `TaskMQTT(20)` vs `TaskSensors(1)`

Si `TaskMQTT` est mis à priorité 20 et `TaskSensors` à 1, le système **ne sera pas meilleur**.
Au contraire : les communications réseau impliquent des temps d'attente matériels importants. Avec une priorité de 20, le traitement MQTT préemptera la lecture des capteurs. En cas de perte de connexion ou de ralentissement TCP, la station IoT cessera de lire l'état du réacteur, ce qui annule toute notion de sécurité temps-réel. La règle d'or est le "Rate Monotonic Scheduling" : les tâches aux délais d'exécution les plus courts et les plus critiques doivent avoir les priorités les plus hautes.

## 7. Protection de variable globale partagée

```cpp
int temperature;
TaskSensors: temperature = readTemp();
TaskWeb:     Serial.println(temperature);
```
Ce code n'est pas thread-safe (non correct d'un point de vue RTOS).
Si la lecture ou l'écriture n'est pas atomique au niveau du processeur (ce qui est souvent le cas sur les struct ou flottants), ou si deux cœurs (ESP32) accèdent à la RAM simultanément, il y a un risque de corruption de données (data race). La tâche web pourrait lire une donnée à moitié modifiée.

## 8. Mutex, Queue, ou Sémaphore

*   **Mutex (Mutual Exclusion)** : C'est un jeton unique (verrou) utilisé pour protéger une ressource partagée. Une seule tâche peut le posséder à la fois.
*   **Queue** : Une file d'attente FIFO permettant d'échanger des données entre différentes tâches de manière sûre (Thread-Safe).
*   **Sémaphore** : Ressemble à un Mutex mais agit plutôt comme un compteur (pour limiter un nombre de ressources) ou un signal d'une tâche à une autre (synchronisation).

**Quand utiliser un Mutex plutôt qu'une Queue ?**
On utilise un Mutex lorsque plusieurs tâches ont besoin de lire ou d'écrire l'état actuel d'une structure (ex: `ReactorState`), sans notion d'historique. 
On utilise une Queue lorsqu'on veut passer un flux continu d'événements (ex: une suite de trames JSON à envoyer) d'une tâche productrice à une tâche consommatrice.

## 9. Pourquoi une Queue serait meilleure pour passer une température ?

```cpp
TaskSensors { temp = read(); }
TaskMQTT { publish(temp); }
```
Dans ce cas précis, utiliser une **Queue** est bien meilleur qu'un Mutex.
Si le réseau est lent, le `publish` va prendre du temps. Avec un Mutex, `TaskSensors` serait bloquée en attendant de pouvoir écrire la nouvelle valeur. Avec une Queue, `TaskSensors` pousse ses valeurs dans la file et repart immédiatement faire autre chose. `TaskMQTT` dépile les valeurs à son rythme. Cela garantit un découplage total et l'absence de perte de points de données.

## 10. La "Starvation" (Famine)

La *starvation* se produit lorsqu'une ou plusieurs tâches de faible priorité n'obtiennent jamais l'accès au CPU, car des tâches de plus haute priorité sont toujours "Ready" et accaparent tout le temps machine.
**Exemple** : Une tâche `TaskCompute` de priorité 3 qui fait une boucle de calcul cryptographique infinie sans aucun `vTaskDelay`. Si une tâche `TaskBlinkLED` de priorité 1 essaie de s'exécuter, le scheduler ne lui donnera jamais la main. La LED ne clignotera jamais.

## 11. Bilan : FreeRTOS vs Boucle Arduino

Une boucle classique `loop()` Arduino traite chaque instruction de façon strictement séquentielle. Si la fonction qui s'occupe du client WiFi se bloque pendant 3 secondes lors d'une reconnexion (timeout classique), tout le programme est gelé. Aucun capteur n'est lu, le bouton d'arrêt d'urgence est mort.
Dans un contexte industriel comme ce TP (système sécurisé et autonome), ce comportement est inacceptable. FreeRTOS garantit le temps-réel (RTOS) : grâce à la préemption temporelle, la tâche de contrôle s'exécutera toujours toutes les 50 ms avec une précision absolue, peu importe si le réseau MQTT plante, car sa priorité de 4 impose au scheduler d'interrompre toute autre tâche défaillante.
