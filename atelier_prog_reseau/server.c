#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#define MAX_CLIENTS 100
#define HISTORY_FILE "chat_history.log"

// Structure pour représenter un client connecté
typedef struct {
    int socket_fd;
    char pseudo[MAX_PSEUDO_LEN];
    time_t connection_time;
    struct sockaddr_in address;
} ClientNode;

// Variables globales partagées
ClientNode clients[MAX_CLIENTS];
int client_count = 0;

// Mutex pour synchroniser l'accès à la liste globale des clients et à la console/log
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Fonction utilitaire pour journaliser l'activité dans la console et dans le fichier historique
void log_activity(const char *message) {
    pthread_mutex_lock(&log_mutex);
    
    // Obtenir l'heure courante
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Afficher dans la console du serveur
    printf("[%s] %s\n", time_str, message);
    fflush(stdout);

    // Écrire dans le fichier historique
    FILE *log_file = fopen(HISTORY_FILE, "a");
    if (log_file != NULL) {
        fprintf(log_file, "[%s] %s\n", time_str, message);
        fclose(log_file);
    }
    
    pthread_mutex_unlock(&log_mutex);
}

// Fonction pour diffuser un message à tous les clients connectés (sauf l'expéditeur facultatif)
void broadcast_message(const ChatMessage *msg, const char *exclude_pseudo) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (exclude_pseudo == NULL || strcmp(clients[i].pseudo, exclude_pseudo) != 0) {
            if (send_all(clients[i].socket_fd, msg, sizeof(ChatMessage)) < 0) {
                perror("[Serveur] Erreur de diffusion");
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Fonction pour envoyer un message privé à un utilisateur spécifique
int send_private_message(const ChatMessage *msg) {
    int found = 0;
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].pseudo, msg->recipient) == 0) {
            if (send_all(clients[i].socket_fd, msg, sizeof(ChatMessage)) < 0) {
                perror("[Serveur] Erreur envoi message privé");
            }
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return found;
}

// Vérifie la validité d'un pseudo (unicité, longueur, caractères)
int validate_pseudo(const char *pseudo) {
    if (strlen(pseudo) == 0 || strlen(pseudo) >= MAX_PSEUDO_LEN) {
        return 0;
    }
    
    // Vérification de l'unicité
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (strcasecmp(clients[i].pseudo, pseudo) == 0) {
            pthread_mutex_unlock(&clients_mutex);
            return 0; // Pseudo déjà pris (insensible à la casse)
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return 1; // Pseudo valide et disponible
}

// Routine exécutée par le thread de chaque client
void *client_handler(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);
    
    char client_pseudo[MAX_PSEUDO_LEN] = {0};
    ChatMessage msg;
    char log_buf[2048];
    
    // 1. Étape d'identification : Attente du message de connexion
    if (recv_all(client_fd, &msg, sizeof(ChatMessage)) <= 0 || msg.type != MSG_CONNECT) {
        close(client_fd);
        return NULL;
    }
    
    // Nettoyer les espaces en début/fin de pseudo
    char clean_pseudo[MAX_PSEUDO_LEN];
    sscanf(msg.sender, "%s", clean_pseudo);
    
    if (!validate_pseudo(clean_pseudo)) {
        // Envoi d'un message d'erreur d'identification
        ChatMessage error_msg;
        error_msg.type = MSG_NOTIFICATION;
        strcpy(error_msg.sender, "SYSTEM");
        strcpy(error_msg.content, "Connexion refusée : pseudo invalide, vide ou déjà pris.");
        error_msg.status = -1;
        
        send_all(client_fd, &error_msg, sizeof(ChatMessage));
        close(client_fd);
        return NULL;
    }
    
    // Enregistrement du client
    pthread_mutex_lock(&clients_mutex);
    if (client_count >= MAX_CLIENTS) {
        pthread_mutex_unlock(&clients_mutex);
        
        ChatMessage error_msg;
        error_msg.type = MSG_NOTIFICATION;
        strcpy(error_msg.sender, "SYSTEM");
        strcpy(error_msg.content, "Connexion refusée : Serveur complet.");
        error_msg.status = -2;
        
        send_all(client_fd, &error_msg, sizeof(ChatMessage));
        close(client_fd);
        return NULL;
    }
    
    // Ajouter le client à notre liste
    strcpy(client_pseudo, clean_pseudo);
    clients[client_count].socket_fd = client_fd;
    strcpy(clients[client_count].pseudo, client_pseudo);
    clients[client_count].connection_time = time(NULL);
    client_count++;
    pthread_mutex_unlock(&clients_mutex);
    
    // Envoyer la confirmation de connexion réussie
    ChatMessage success_msg;
    success_msg.type = MSG_NOTIFICATION;
    strcpy(success_msg.sender, "SYSTEM");
    sprintf(success_msg.content, "Connexion établie ! Bienvenue %s.", client_pseudo);
    success_msg.status = 0;
    send_all(client_fd, &success_msg, sizeof(ChatMessage));
    
    // Journaliser et notifier tous les clients de l'arrivée du nouvel utilisateur
    sprintf(log_buf, "[Système] %s a rejoint le chat.", client_pseudo);
    log_activity(log_buf);
    
    ChatMessage join_notify;
    join_notify.type = MSG_NOTIFICATION;
    strcpy(join_notify.sender, "SYSTEM");
    strcpy(join_notify.content, log_buf);
    broadcast_message(&join_notify, client_pseudo);
    
    // 2. Boucle principale de dialogue
    while (1) {
        int nbytes = recv_all(client_fd, &msg, sizeof(ChatMessage));
        if (nbytes <= 0) {
            // Déconnexion brutale
            break;
        }
        
        if (msg.type == MSG_DISCONNECT) {
            // Déconnexion propre initiée par le client
            break;
        }
        
        switch (msg.type) {
            case MSG_PUBLIC:
                // Forcer l'expéditeur pour des raisons de sécurité
                strcpy(msg.sender, client_pseudo);
                
                // Journaliser le message public
                sprintf(log_buf, "%s (Public) : %s", client_pseudo, msg.content);
                log_activity(log_buf);
                
                // Diffuser à tous les clients
                broadcast_message(&msg, client_pseudo);
                break;
                
            case MSG_PRIVATE:
                strcpy(msg.sender, client_pseudo);
                
                // Journaliser le message privé
                sprintf(log_buf, "%s -> %s (Privé) : %s", client_pseudo, msg.recipient, msg.content);
                log_activity(log_buf);
                
                // Tenter d'envoyer au destinataire
                int sent = send_private_message(&msg);
                if (!sent) {
                    // Envoyer un message d'erreur à l'expéditeur
                    ChatMessage error_feedback;
                    error_feedback.type = MSG_NOTIFICATION;
                    strcpy(error_feedback.sender, "SYSTEM");
                    sprintf(error_feedback.content, "Erreur : L'utilisateur '%s' n'est pas connecté.", msg.recipient);
                    error_feedback.status = -1;
                    send_all(client_fd, &error_feedback, sizeof(ChatMessage));
                }
                break;
                
            case MSG_USERS: {
                // Renvoyer la liste des utilisateurs
                ChatMessage users_msg;
                users_msg.type = MSG_USERS;
                strcpy(users_msg.sender, "SYSTEM");
                
                // Construire la liste formatée
                pthread_mutex_lock(&clients_mutex);
                int offset = 0;
                offset += snprintf(users_msg.content + offset, MAX_CONTENT_LEN - offset, "=== Participants Connectés (%d) ===\n", client_count);
                for (int i = 0; i < client_count; i++) {
                    struct tm *tm_info = localtime(&clients[i].connection_time);
                    char time_str[9];
                    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
                    offset += snprintf(users_msg.content + offset, MAX_CONTENT_LEN - offset, "- %s (connecté depuis %s)\n", clients[i].pseudo, time_str);
                }
                pthread_mutex_unlock(&clients_mutex);
                
                send_all(client_fd, &users_msg, sizeof(ChatMessage));
                break;
            }
            
            default:
                break;
        }
    }
    
    // 3. Phase de déconnexion et de nettoyage
    pthread_mutex_lock(&clients_mutex);
    // Retirer le client du tableau
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket_fd == client_fd) {
            // Remplacer l'élément actuel par le dernier du tableau pour réorganiser
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    // Journaliser et informer les autres de la sortie du client
    sprintf(log_buf, "[Système] %s a quitté le chat.", client_pseudo);
    log_activity(log_buf);
    
    ChatMessage leave_notify;
    leave_notify.type = MSG_NOTIFICATION;
    strcpy(leave_notify.sender, "SYSTEM");
    strcpy(leave_notify.content, log_buf);
    broadcast_message(&leave_notify, NULL);
    
    close(client_fd);
    return NULL;
}

int main(int argc, char *argv[]) {
    int port = 5555; // Port par défaut
    
    // Récupération et parsing du numéro de port
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Usage : %s [port_écoute]\n", argv[0]);
            printf("Par défaut, le port est fixé à 5555.\n");
            exit(EXIT_SUCCESS);
        }
        port = atoi(argv[1]);
    }
    
    // Création de la socket serveur
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Impossible de créer la socket serveur");
        exit(EXIT_FAILURE);
    }
    
    // Réutilisation du port pour éviter l'erreur "Address already in use"
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt a échoué");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Configuration de l'adresse d'écoute
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Association bind
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Le bind a échoué");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Mise en écoute listen
    if (listen(server_fd, 10) < 0) {
        perror("Le listen a échoué");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    char startup_msg[256];
    sprintf(startup_msg, "Serveur Chat Hub démarré avec succès sur le port %d...", port);
    log_activity(startup_msg);
    
    // Boucle infinie pour accepter les connexions clients
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Erreur lors de l'acceptation de la connexion client");
            continue;
        }
        
        // Journaliser la connexion entrante brute
        char log_conn[128];
        sprintf(log_conn, "Tentative de connexion depuis l'IP: %s, Port: %d", 
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        log_activity(log_conn);
        
        // Allouer la mémoire pour transmettre le descripteur au thread
        int *client_fd_ptr = malloc(sizeof(int));
        if (client_fd_ptr == NULL) {
            perror("Erreur malloc pour le socket client");
            close(client_fd);
            continue;
        }
        *client_fd_ptr = client_fd;
        
        // Créer un thread pour ce nouveau client
        pthread_t tid;
        if (pthread_create(&tid, NULL, client_handler, client_fd_ptr) != 0) {
            perror("Erreur de création de thread pour un client");
            close(client_fd);
            free(client_fd_ptr);
            continue;
        }
        
        // Détacher le thread pour qu'il libère automatiquement ses ressources à sa mort
        pthread_detach(tid);
    }
    
    close(server_fd);
    return 0;
}
