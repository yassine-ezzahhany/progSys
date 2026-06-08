#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#define MAX_PSEUDO_LEN 32
#define MAX_CONTENT_LEN 1024

// Types de messages supportés par le protocole
typedef enum {
    MSG_CONNECT,      // Demande de connexion du client avec pseudo
    MSG_DISCONNECT,   // Notification de déconnexion propre du client
    MSG_PUBLIC,       // Message diffusé à tous les clients
    MSG_PRIVATE,      // Message privé envoyé à un destinataire spécifique
    MSG_USERS,        // Requête ou réponse de la liste des utilisateurs connectés
    MSG_NOTIFICATION  // Notification du système (alertes, connexions, déconnexions, erreurs)
} MsgType;

// Structure de message de taille fixe pour éviter la fragmentation TCP
typedef struct {
    MsgType type;
    char sender[MAX_PSEUDO_LEN];
    char recipient[MAX_PSEUDO_LEN]; // Rempli uniquement pour les MSG_PRIVATE
    char content[MAX_CONTENT_LEN];
    int status; // 0 = OK, non-zéro pour signaler une erreur (ex: pseudo déjà pris)
} ChatMessage;

// Fonctions réseau utilitaires pour garantir l'atomicité des transferts TCP
int send_all(int socket_fd, const void *buf, size_t len);
int recv_all(int socket_fd, void *buf, size_t len);

#endif // COMMON_H
