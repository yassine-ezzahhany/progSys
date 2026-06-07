# Rapport de TP : Programmation Système et Réseau
### Sockets TCP et Client-Serveur de Chat Multi-threadé (Chat Hub)

---

**Réalisé par :** EZZAHHANY YASSINE  
**Filière / Niveau :** FIGI (Filière Ingénieur Génie Informatique) - Semestre 4 (S4)  
**Établissement :** Faculté des Sciences et Techniques (FST) Settat, Université Hassan 1er  
**Date :** Juin 2026  

---

## 📖 Introduction générale

Ce TP s'inscrit dans le cadre du module **Programmation Système et Réseau**. Après avoir étudié la communication inter-processus locale via les tubes (Pipes) et la capture des signaux système, ce travail pratique aborde la communication à travers le réseau à l'aide des **Sockets BSD** sous Unix.

L'objectif de ce TP est de concevoir et de comparer deux modèles d'applications de messagerie instantanée (Chat) de type client-serveur basées sur le protocole **TCP** (SOCK_STREAM) :
1. **Un modèle bloquant synchrone (`client.c` et `server.c`)** : où le client et le serveur alternent les rôles de lecteur et d'écrivain de façon séquentielle. Nous analyserons les limites intrinsèques de ce modèle face aux exigences de l'interactivité en temps réel.
2. **Un modèle asynchrone multi-threadé (`client_nb.c` et `server_nb.c`)** : qui résout le blocage de l'interface en séparant la lecture réseau et la saisie utilisateur dans des threads POSIX (`pthread`) distincts, tout en gérant l'affichage de la console en mode terminal brut (`termios`) et la synchronisation avec des verrous d'exclusion mutuelle (`mutex`).

Ce rapport présente l'analyse détaillée des concepts théoriques, l'explication des structures de code C, la mise en évidence des limitations fonctionnelles et la justification de la solution asynchrone multi-threadée.

---

## 🔌 Partie 1 : Messagerie TCP Bloquante (Synchrone)

### 1. Concept de la Communication par Socket TCP
Le protocole **TCP** (Transmission Control Protocol) est un protocole orienté connexion, fiable et bidirectionnel (Full-Duplex). La mise en relation d'un client et d'un serveur TCP repose sur une machine à états stricte utilisant des appels système POSIX :
- **Côté Serveur** :
  1. `socket()` : Allocation du descripteur de socket réseau.
  2. `bind()` : Association de la socket à un couple (Adresse IP, Port).
  3. `listen()` : Configuration de la file d'attente pour accueillir les demandes de connexion entrantes.
  4. `accept()` : Appel bloquant qui attend la connexion d'un client et retourne un nouveau descripteur de fichier spécifique à cette session de communication.
- **Côté Client** :
  1. `socket()` : Création de la socket cliente.
  2. `connect()` : Lancement de la poignée de main TCP (3-way handshake) vers le serveur spécifié par son adresse IP et son port.

```text
       SERVEUR                                          CLIENT
   +------------+                                   +------------+
   |  socket()  |                                   |  socket()  |
   +-----+------+                                   +-----+------+
         |                                                |
   +-----+------+                                         |
   |   bind()   |                                         |
   +-----+------+                                         |
         |                                                |
   +-----+------+                                         |
   |  listen()  |                                         |
   +-----+------+                                         |
         |                                                |
   +-----+------+            Connexion                    |
   |  accept()  |<--------------------------------------+-| connect()  |
   +-----+------+    (Poignée de main TCP à 3 voies)      +------------+
         |                                                      |
         v                                                      v
  [Socket connectée]                                    [Socket connectée]
         |                                                      |
   +-----+------+       Échange séquentiel (Ping-pong)   +-----+------+
   |    read()  |<======================================|   write()  |
   +-----+------+                                       +-----+------+
         |                                                      |
   +-----+------+                                       +-----+------+
   |   write()  |======================================>|    read()  |
   +-----+------+                                       +-----+------+
```

---

### 2. Fonctionnement du Programme Synchrone (`client.c` et `server.c`)

Dans cette première version, le client et le serveur s'échangent des messages textuels en mode "ping-pong". L'interaction est régie par la structure classique suivante :

#### A. Le Serveur (`server.c`)
Le serveur écoute sur une adresse IP et un port fournis en ligne de commande. Une fois la connexion acceptée, il appelle la fonction `shats()` qui gère l'échange de messages :
```c
void shats(int fd)
{
  char buffer[256];
  int n;

  while (1)
  {
    bzero(buffer, 256);
    n = read(fd, buffer, 255); // Appel bloquant : attente du message du client
    if (n <= 0) { ... break; }
    printf("Client : %s", buffer);
    
    bzero(buffer, 256);
    printf("Server : ");
    fgets(buffer, 255, stdin); // Appel bloquant : attente de la saisie clavier de l'opérateur

    if (strncmp(buffer, "exit", 4) == 0) break;
    n = write(fd, buffer, strlen(buffer)); // Envoi au client
    if (n < 0) error("ERROR writing to socket");
  }
}
```

#### B. Le Client (`client.c`)
Le client se connecte au serveur et exécute une boucle complémentaire :
```c
  while (1) {
    printf("client : ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin); // Appel bloquant : attente de la saisie utilisateur

    if (strncmp(buffer, "exit", 4) == 0) break;
    n = write(socket_fd, buffer, strlen(buffer)); // Envoi au serveur
    
    bzero(buffer, 256);
    n = read(socket_fd, buffer, 255); // Appel bloquant : attente de la réponse du serveur
    if (n <= 0) { ... break; }
    printf("server : %s", buffer);
  }
```

---

### 3. Limites du Modèle Bloquant Synchrone

Bien que ce code fonctionne pour un échange simple alterné, il souffre de limitations critiques dues au **blocage des appels système d'E/S (Entrées/Sorties)** :

1. **Le Syndrome du Ping-Pong Imposé** : L'utilisation de `fgets()` (lecture clavier) et de `read()` (lecture réseau) s'effectue sur le même thread unique. Si le serveur attend que l'utilisateur saisisse son texte sur `stdin` (bloqué dans `fgets`), il est **totalement sourd** aux messages envoyés par le client via le réseau. Le client peut envoyer plusieurs lignes, elles resteront en attente dans la file réseau et ne s'afficheront que lorsque l'opérateur du serveur aura appuyé sur `Entrée`.
2. **Impossibilité d'Émettre en Continu** : Un utilisateur ne peut pas envoyer deux messages d'affilée sans attendre une réponse, car le code repasse immédiatement dans un appel `read()` bloquant après chaque `write()`.
3. **Monoclientèle** : Le serveur ne peut gérer qu'un seul client à la fois. Si un deuxième client tente de se connecter, sa demande est mise en attente au niveau de la file d'écoute TCP (`backlog` de `listen`) et il ne sera traité que lorsque le premier client aura fermé sa connexion.

---

## 🧵 Partie 2 : Messagerie TCP Asynchrone / Multi-threadée

Pour concevoir un véritable "Chat Hub" interactif en temps réel, il est nécessaire d'écouter simultanément deux sources d'événements asynchrones : la **saisie clavier** et la **réception réseau**. La solution implémentée dans `client_nb.c` et `server_nb.c` utilise le **multithreading** avec la bibliothèque standard `pthread` ainsi qu'une manipulation bas niveau du terminal.

### 1. Architecture Multi-threadée (Séparation des flux)
Pour chaque connexion active, le programme lance deux threads indépendants qui s'exécutent en parallèle et partagent la socket de communication :

1. **Le Thread Lecteur (`shats_reader` / `client_reader`)** :
   Il s'exécute dans une boucle infinie et effectue un appel `read()` bloquant sur la socket réseau. Dès que des données arrivent du réseau, le thread les traite et les affiche immédiatement à l'écran. Il ne dépend pas et n'attend pas d'activité sur le clavier.
2. **Le Thread Écrivain (`shats_writer` / `client_writer`)** :
   Il surveille l'entrée standard clavier (`STDIN_FILENO`). Dès que l'utilisateur valide sa saisie, il transmet les données au destinataire via la socket à l'aide de l'appel `write()`.

```text
                  +---------------------------------------+
                  |           PROCESSUS CHAT              |
                  |                                       |
                  |     +---------------------------+     |
                  |     | Thread Lecteur (Reader)   |     |
                  |     | - read() sur Socket       |     |
                  |     +-------------+-------------+     |
                  |                   ^ (Lecture)         |
                  |                   |                   |
                  |             [ SOCKET TCP ]            |
                  |                   |                   |
                  |                   v (Écriture)        |
                  |     +-------------+-------------+     |
                  |     | Thread Écrivain (Writer)  |     |
                  |     | - Saisie clavier raw      |     |
                  |     +---------------------------+     |
                  +---------------------------------------+
```

---

### 2. Le Mode Terminal Brut (`termios`) et la Saisie Asynchrone

Par défaut, les terminaux Unix fonctionnent en **mode canonique** (mode ligne) : les caractères saisis sont stockés par l'OS et ne sont transmis au programme que lorsque l'utilisateur appuie sur `Entrée`. De plus, le terminal réaffiche automatiquement les touches enfoncées (**Echo**).

Dans un chat asynchrone, ce comportement pose problème : si le lecteur reçoit un message du correspondant pendant que l'utilisateur est en train de taper sa propre ligne, les caractères reçus vont s'entremêler visuellement avec les caractères en cours de saisie, rendant l'interface illisible.

Pour résoudre cela, le programme configure le terminal en **mode non canonique sans écho (mode brut)** :
```c
void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  struct termios raw = orig_termios;
  // Désactive ICANON (mode ligne) et ECHO (affichage automatique des touches)
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}
```

Grâce à ce mode brut :
- Le programme intercepte chaque touche instantanément : `read(STDIN_FILENO, &c, 1)`.
- C'est le programme qui gère manuellement l'affichage à l'écran (`putchar(c)`), la touche Retour arrière (`Backspace` modélisée par `\b \b` pour effacer les caractères) et la touche Entrée (`\n`).
- Le texte tapé par l'utilisateur est stocké au fur et à mesure dans un buffer local dynamique (`args->input_buffer`).

---

### 3. Synchronisation et Cohérence de l'Affichage (`Mutex`)

Puisque le thread lecteur et le thread écrivain peuvent vouloir écrire sur l'écran (la sortie standard) en même temps, il y a un risque important de concurrence et de corruption graphique.

Pour éviter cela, les deux threads partagent un **verrou d'exclusion mutuelle (`pthread_mutex_t`)** pour assurer l'atomicité des opérations d'affichage :
- **Lorsqu'un message réseau arrive** :
  Le thread lecteur verrouille le mutex, efface la ligne courante où l'utilisateur était en train de taper grâce au code d'échappement ANSI `\r\033[K` (retour chariot + effacement de la ligne), affiche le message reçu du correspondant, réaffiche proprement le prompt d'écriture (`client : ` ou `Server : `) et redessine exactement le texte que l'utilisateur avait commencé à taper avant l'interruption. Enfin, il déverrouille le mutex.
  
```c
    pthread_mutex_lock(&args->lock);
    printf("\r\033[K");                     // Efface la ligne de saisie courante
    printf("server : %s\n", buffer);        // Affiche le message reçu
    printf("client : %s", args->input_buffer); // Redessine la saisie de l'utilisateur
    fflush(stdout);
    pthread_mutex_unlock(&args->lock);
```

- **Lors de la saisie utilisateur** :
  Chaque caractère tapé ou effacé par l'utilisateur met à jour le buffer local et modifie l'affichage sous la protection du mutex pour garantir qu'aucun message reçu ne vienne s'insérer au milieu de l'opération d'affichage.

---

## 📈 Conclusion générale

Ce travail pratique sur la programmation réseau à travers le développement d'un **Chat Hub TCP** a permis d'opposer deux paradigms de programmation système :

1. **Le modèle bloquant synchrone** simple d'écriture mais souffrant de limitations insurmontables en interactivité temps réel. Il montre que la programmation réseau ne peut pas se contenter d'un flux d'exécution séquentiel unique lorsque plusieurs canaux d'E/S indépendants doivent être gérés en parallèle.
2. **Le modèle multi-threadé asynchrone** qui tire parti de la puissance de la programmation concourante. L'utilisation combinée des threads POSIX (`pthread`), de la reconfiguration dynamique des terminaux (`termios`), et de la synchronisation fine par verrous d'exclusion mutuelle (`mutex`) fournit une solution robuste et élégante.

Ces concepts constituent la base du développement des serveurs réseau modernes de production (serveurs de jeux, serveurs HTTP de chat ou serveurs WebSockets) qui s'appuient sur le multi-threading ou sur des bibliothèques d'I/O asynchrones événementielles (comme `select()`, `poll()` ou `epoll()` sous Linux).
