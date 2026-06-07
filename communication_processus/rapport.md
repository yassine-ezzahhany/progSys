# Rapport de TP : Programmation Système et Réseau
### Signaux et Communication Inter-Processus (Tubes)

---

**Réalisé par :** EZZAHHANY YASSINE  
**Filière / Niveau :** FIGI (Filière Ingénieur Génie Informatique) - Semestre 4 (S4)  
**Établissement :** Faculté des Sciences et Techniques (FST) Settat, Université Hassan 1er  
**Date :** Juin 2026  

---

## 📖 Introduction générale

Ce TP s'inscrit dans le cadre du module **Programmation Système et Réseau**. L'objectif principal est d'appréhender et d'expérimenter deux mécanismes fondamentaux de la gestion des processus sous les systèmes d'exploitation de type Unix/Linux :
1. **La gestion et la capture des signaux** : à travers l'interception du signal d'interruption `SIGINT` (généré par Ctrl-C sur le terminal) afin de modifier le comportement par défaut d'un processus et d'exécuter des opérations de sauvegarde de sécurité avant la terminaison.
2. **La communication inter-processus (IPC) via les tubes POSIX (Pipes)** : en analysant les comportements d'écriture et de lecture dans des tubes unidirectionnels et bidirectionnels, et en comprenant la gestion du signal `SIGPIPE` émis par le noyau lorsque des anomalies de communication surviennent.

Ce rapport présente l'analyse détaillée, les codes sources robustes en langage C, la justification théorique et pratique ainsi que les résultats attendus pour chacun des 3 exercices du TP.

---

## ⚡ Exercice 1 : Capture de SIGINT et Sauvegarde de Données

### 1. Analyse et Objectif
Dans un système d'exploitation Unix, la combinaison de touches `Ctrl-C` dans le terminal envoie le signal `SIGINT` (Signal d'interruption, de numéro 2) au processus en cours d'exécution au premier plan. Par défaut, la réception de ce signal provoque l'arrêt immédiat et brutal du programme.

L'objectif de cet exercice est d'intercepter ce signal pour exécuter un traitement personnalisé avant l'arrêt du programme : sauvegarder l'état actuel ou des données de travail dans un fichier nommé `sauvegarde.txt` (qui doit être créé ou écrasé s'il existe déjà), puis quitter proprement.

### 2. Spécification de la Robustesse et Sécurité Système
Pour garantir un code de niveau professionnel, deux aspects techniques majeurs sont mis en œuvre :
- **Utilisation de `sigaction` au lieu de `signal`** : L'appel système standard POSIX `sigaction` est privilégié car il offre un contrôle fin sur la manipulation des signaux et évite les comportements non portables ou les réinitialisations automatiques des gestionnaires de signaux présents avec l'ancienne fonction `signal`.
- **Fonctions "Async-Signal-Safe" dans le gestionnaire** : Selon la norme POSIX, un gestionnaire de signal peut interrompre le programme principal à n'importe quel instant (par exemple, au milieu d'un appel à `printf` ou `malloc`). Utiliser des fonctions non réentrantes comme `fopen`, `fprintf` ou `printf` dans le gestionnaire peut corrompre la mémoire ou provoquer des blocages (deadlocks). Ainsi, nous utilisons exclusivement les appels système bas niveau `open`, `write` et `close`, qui sont documentés comme sûrs et atomiques en contexte de signal.

### 3. Code Source (`exercice1.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

// Variable globale volatile pour garantir l'accès direct et atomique lors du signal
volatile sig_atomic_t iteration_count = 0;

// Conversion manuelle d'un entier en chaîne (conforme "async-signal-safe")
void int_to_str(int n, char *str) {
    int i = 0;
    if (n == 0) {
        str[i++] = '0';
    } else {
        while (n > 0) {
            str[i++] = (n % 10) + '0';
            n /= 10;
        }
    }
    str[i] = '\0';
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - 1 - j];
        str[i - 1 - j] = temp;
    }
}

// Gestionnaire du signal SIGINT
void handle_sigint(int sig) {
    (void)sig; // Éviter l'avertissement de compilation
    
    const char *filename = "sauvegarde.txt";
    
    // O_WRONLY | O_CREAT | O_TRUNC : Remplace/crée le fichier de sauvegarde
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        const char *err = "Erreur de création de sauvegarde.txt\n";
        write(STDERR_FILENO, err, strlen(err));
        exit(EXIT_FAILURE);
    }

    const char *header = "=== SAUVEGARDE ETAT PROGRAMME ===\n";
    const char *status = "Statut : Arrêté via SIGINT (Ctrl-C)\n";
    const char *iter_msg = "Derniere iteration atteinte : ";

    write(fd, header, strlen(header));
    write(fd, status, strlen(status));
    write(fd, iter_msg, strlen(iter_msg));
    
    char count_buf[12];
    int_to_str(iteration_count, count_buf);
    write(fd, count_buf, strlen(count_buf));
    write(fd, "\n=================================\n", 35);
    
    close(fd);

    const char *console = "\n[Signal] SIGINT intercepte. Sauvegarde dans 'sauvegarde.txt' OK. Arrêt.\n";
    write(STDOUT_FILENO, console, strlen(console));
    
    exit(EXIT_SUCCESS);
}

int main() {
    printf("=========================================================\n");
    printf("  Exercice 1 : Capture de SIGINT et Sauvegarde de données\n");
    printf("=========================================================\n");
    printf("[PID : %d] Programme en cours. Appuyez sur Ctrl-C...\n\n", getpid());

    // Liaison du signal SIGINT à notre gestionnaire propre via sigaction
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Erreur sigaction");
        return EXIT_FAILURE;
    }

    // Tâche fictive infinie incrémentant un compteur de progression
    while (1) {
        iteration_count++;
        printf("  -> Etape %d en cours...\n", iteration_count);
        sleep(1);
    }

    return EXIT_SUCCESS;
}
```

### 4. Explication des résultats
- Au lancement, le programme affiche son PID et incrémente son compteur d'itérations toutes les secondes sur la console.
- Lorsque l'utilisateur appuie sur `Ctrl-C` (qui envoie `SIGINT`), l'exécution normale du programme principal est mise en pause. Le processeur bascule immédiatement sur le gestionnaire `handle_sigint`.
- Le gestionnaire ouvre le fichier `sauvegarde.txt`, écrit le message avec le nombre d'itérations atteintes (grâce à notre fonction de formatage réentrante `int_to_str`), ferme le fichier, affiche un message à l'écran puis appelle `exit()`.
- Le fichier `sauvegarde.txt` contient alors :
  ```text
  === SAUVEGARDE ETAT PROGRAMME ===
  Statut : Arrêté via SIGINT (Ctrl-C)
  Derniere iteration atteinte : 5
  =================================
  ```
- Si l'on relance le programme et l'interrompt à nouveau, le fichier est écrasé proprement avec la nouvelle valeur d'itération grâce à l'option `O_TRUNC`.

---

## 🪓 Exercice 2 : Écriture dans un tube sans lecteur (Analyse de SIGPIPE)

### 1. Analyse du Code Fourni
Le code fourni réalise les étapes successives suivantes :
1. Déclaration d'un tube représenté par un tableau de deux descripteurs de fichiers : `tube[2]`.
2. Création du tube via l'appel système `pipe(tube)`. `tube[0]` devient l'extrémité de lecture et `tube[1]` devient l'extrémité d'écriture.
3. Fermeture explicite du descripteur de lecture du tube : `close(tube[0])`. À cet instant précis, le tube possède une extrémité d'écriture active (`tube[1]`), mais **plus aucun processus lecteur n'est connecté à son extrémité de lecture**.
4. Tentative d'écriture de la chaîne `"AZERTYUIOP"` (longueur 10 octets) sur l'extrémité d'écriture restante : `write(tube[1], buffer, strlen(buffer))`.

### 2. Comportement du Système et Signal Déclenché
En programmation système Unix, le comportement d'un tube obéit à des règles strictes régies par le noyau :
- Si un processus tente d'écrire dans un tube alors qu'il n'y a plus aucun descripteur ouvert en lecture sur ce tube (c'est-à-dire que tous les lecteurs ont fermé leur accès ou se sont terminés), l'écriture n'a aucun sens car les données ne pourront jamais être consommées.
- Pour éviter de bloquer indéfiniment le processus écrivain ou de gaspiller de la mémoire tampon du noyau, le système d'exploitation envoie immédiatement le signal **`SIGPIPE` (Signal numéro 13 - Broken Pipe)** au processus fautif.
- Par défaut, le comportement associé au signal `SIGPIPE` est la **terminaison immédiate** du processus.

### 3. Réponse à la question : "En cas d'erreur, le code fait objet de quel signal ?"
Le code fait l'objet du signal **`SIGPIPE`** (Signal de rupture de tube, valeur numérique standard **13**).

### 4. Conséquence sur l'exécution du programme
Puisque le signal `SIGPIPE` est envoyé dès l'exécution de l'appel `write()`, et qu'aucun gestionnaire de signal n'a été installé pour le capturer ou l'ignorer, le programme est interrompu instantanément par le noyau.
Ainsi, la ligne suivante :
```c
fprintf(stdout, "Fin du programme \n");
```
n'est **jamais exécutée** et aucun message de fin ne s'affiche sur la sortie standard. Le code de retour du programme dans le shell correspondra à l'interruption par signal (généralement $128 + 13 = 141$).

---

## 🔁 Exercice 3 : Communication Bidirectionnelle Père/Fils via Tubes

### 1. Analyse et Architecture de Communication
Un tube Unix (`pipe`) est intrinsèquement unidirectionnel : les données entrent par l'extrémité d'écriture (indice `1`) et sortent par l'extrémité de lecture (indice `0`).
Pour concevoir une communication **bidirectionnelle** (Half-Duplex ou Full-Duplex) entre un processus père et son fils, il est indispensable de créer **deux tubes distincts** :
1. **Le premier tube `p1`** : gère le flux de données allant du **père vers le fils**.
   - Le père ferme l'extrémité de lecture `p1[0]` et écrit dans `p1[1]`.
   - Le fils ferme l'extrémité d'écriture `p1[1]` et lit dans `p1[0]`.
2. **Le second tube `p2`** : gère le flux de données allant du **fils vers le père**.
   - Le fils ferme l'extrémité de lecture `p2[0]` et écrit dans `p2[1]`.
   - Le père ferme l'extrémité d'écriture `p2[1]` et lit dans `p2[0]`.

### 2. Algorithme d'échange (Gestionnaire)
1. Le père initialise les deux tubes `p1` et `p2` puis duplique son espace mémoire avec `fork()`.
2. **Le Fils (Lecteur/Calculateur)** :
   - Ferme les canaux inutilisés (`p1[1]` et `p2[0]`).
   - Effectue une boucle de lecture de 5 entiers depuis `p1[0]`.
   - À chaque réception d'un entier, il l'affiche sur l'écran, multiplie sa valeur par $2$ pour obtenir le double, puis écrit ce résultat sur `p2[1]`.
   - Après 5 itérations, il ferme ses descripteurs et se termine proprement.
3. **Le Père (Émetteur/Récepteur final)** :
   - Ferme les canaux inutilisés (`p1[0]` et `p2[1]`).
   - Parcourt un tableau de 5 entiers de test. Pour chaque entier :
     - Il l'écrit sur `p1[1]`.
     - Il se bloque en lecture sur `p2[0]` en attendant le retour du double calculé par le fils.
     - Dès réception, il affiche le double sur l'écran.
   - Utilise `wait(NULL)` pour s'assurer que le fils s'est éteint proprement sans laisser de processus zombie.
   - Affiche un tableau récapitulatif des calculs et ferme ses canaux.

### 3. Schéma structurel des liaisons
```text
           +---------------------------------------------+
           |               PROCESSUS PÈRE                |
           +----------------------+----------------------+
                                  | (Écriture)
                                  v
                            [ Tube P1[1] ] 
                                  |
                                  |  (Transmission entiers)
                                  v
                            [ Tube P1[0] ]
                                  |
                                  v (Lecture)
           +----------------------+----------------------+
           |               PROCESSUS FILS                |
           |   (Affiche l'entier & calcule le double)    |
           +----------------------+----------------------+
                                  | (Écriture)
                                  v
                            [ Tube P2[1] ]
                                  |
                                  |  (Transmission doubles)
                                  v
                            [ Tube P2[0] ]
                                  |
                                  v (Lecture)
           +----------------------+----------------------+
           |               PROCESSUS PÈRE                |
           |         (Affiche le double final)           |
           +---------------------------------------------+
```

### 4. Code Source (`exercice3.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int p1[2]; // Tube Père -> Fils
    int p2[2]; // Tube Fils -> Père

    printf("=======================================================================\n");
    printf("  Exercice 3 : Communication bidirectionnelle via tubes (Père <-> Fils) \n");
    printf("=======================================================================\n");

    // Création des deux tubes
    if (pipe(p1) != 0 || pipe(p2) != 0) {
        perror("Erreur lors de l'initialisation des tubes");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Erreur fork");
        return EXIT_FAILURE;
    } 
    else if (pid == 0) {
        // --- PROCESSUS FILS ---
        close(p1[1]); // Ferme écriture p1
        close(p2[0]); // Ferme lecture p2

        printf("[Fils - PID: %d] Prêt à lire sur p1 et écrire sur p2.\n", getpid());
        
        int val_recue;
        int val_double;

        for (int i = 0; i < 5; i++) {
            // Lecture bloquante de l'entier
            if (read(p1[0], &val_recue, sizeof(int)) <= 0) {
                perror("[Fils] Erreur lecture");
                exit(EXIT_FAILURE);
            }
            printf("[Fils - PID: %d] Reçu : %d\n", getpid(), val_recue);

            // Traitement
            val_double = val_recue * 2;

            // Écriture du résultat
            if (write(p2[1], &val_double, sizeof(int)) <= 0) {
                perror("[Fils] Erreur écriture");
                exit(EXIT_FAILURE);
            }
        }

        close(p1[0]);
        close(p2[1]);
        printf("[Fils - PID: %d] Travail terminé.\n", getpid());
        exit(EXIT_SUCCESS);
    } 
    else {
        // --- PROCESSUS PÈRE ---
        close(p1[0]); // Ferme lecture p1
        close(p2[1]); // Ferme écriture p2

        int valeurs_test[5] = {5, 12, 23, 42, 99};
        int resultats[5];

        printf("[Père - PID: %d] Début de la transmission bidirectionnelle...\n\n", getpid());

        for (int i = 0; i < 5; i++) {
            printf("[Père] Envoi : %d\n", valeurs_test[i]);
            
            // Envoi de la valeur brute au fils
            if (write(p1[1], &valeurs_test[i], sizeof(int)) <= 0) {
                perror("[Père] Erreur écriture");
                return EXIT_FAILURE;
            }

            // Réception immédiate du double calculé
            if (read(p2[0], &resultats[i], sizeof(int)) <= 0) {
                perror("[Père] Erreur lecture");
                return EXIT_FAILURE;
            }
            printf("[Père] Reçu en retour : %d\n\n", resultats[i]);
        }

        // Attente de la terminaison du processus fils
        wait(NULL);
        
        close(p1[1]);
        close(p2[0]);

        // Affichage du bilan
        printf("=======================================================================\n");
        printf("[Bilan Père] Validation finale des calculs :\n");
        for (int i = 0; i < 5; i++) {
            printf("  - Entrée: %2d  ==>  Sortie Fils: %3d (Validation: %s)\n", 
                   valeurs_test[i], resultats[i], 
                   (valeurs_test[i] * 2 == resultats[i]) ? "OK" : "ÉCHEC");
        }
        printf("=======================================================================\n");
    }

    return EXIT_SUCCESS;
}
```

---

## 📈 Conclusion générale

Ce troisième TP de programmation système Unix a mis en lumière les mécanismes fondamentaux qui permettent de synchroniser et de faire échanger des données à des processus indépendants :
- **La capture et le déroutement de signaux** nous ont appris à rendre nos programmes robustes et capables de réagir à des événements externes imprévus (comme une interruption Ctrl-C) sans perte d'information, tout en respectant scrupuleusement les contraintes de sécurité POSIX (notamment la réentrance et l'async-signal-safety).
- **L'utilisation des tubes (`pipe`)** a illustré la communication inter-processus élémentaire en mémoire partagée par le noyau. L'exercice bidirectionnel a démontré la nécessité de concevoir des topologies claires de descripteurs (deux tubes pour éviter les blocages croisés ou deadlocks) et d'assurer une gestion rigoureuse de la fermeture des extrémités inutilisées afin d'éviter les fuites de ressources ou les crashes par rupture de flux (`SIGPIPE`).

Ces concepts d'IPC et de traitement des événements asynchrones constituent le fondement indispensable pour aborder par la suite la programmation réseau (Sockets TCP/UDP) et le développement d'applications distribuées de type client-serveur.
