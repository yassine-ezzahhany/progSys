#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_RED     "\033[31m"
#define COLOR_MAGENTA "\033[35m"

// Fonctions utilitaires d'affichage console premium
void clear_line() {
    printf("\r\033[2K");
    fflush(stdout);
}

void draw_prompt(const char *pseudo) {
    printf("%s%s> %s", COLOR_CYAN, pseudo, COLOR_RESET);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    char server_ip[64] = "127.0.0.1";
    int port = 5555;
    char my_pseudo[MAX_PSEUDO_LEN] = {0};
    
    // Parsing élaboré des arguments (ex: --server localhost --port 5555)
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 && i + 1 < argc) {
            strcpy(server_ip, argv[i + 1]);
            // Convertir 'localhost' en 127.0.0.1
            if (strcmp(server_ip, "localhost") == 0) {
                strcpy(server_ip, "127.0.0.1");
            }
            i++;
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage : %s [--server <IP_ou_localhost>] [--port <port>]\n", argv[0]);
            exit(EXIT_SUCCESS);
        }
    }
    
    // Création du socket client
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Erreur de création du socket client");
        exit(EXIT_FAILURE);
    }
    
    // Configuration de l'adresse du serveur
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "%s[ERREUR] Adresse IP '%s' invalide ou non supportée.%s\n", COLOR_RED, server_ip, COLOR_RESET);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    // Connexion au serveur
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("La connexion au serveur a échoué");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    // Étape d'identification : Demander le pseudo à l'utilisateur
    printf("Pseudo : ");
    fflush(stdout);
    if (fgets(my_pseudo, sizeof(my_pseudo), stdin) == NULL) {
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    // Nettoyage du pseudo
    my_pseudo[strcspn(my_pseudo, "\n")] = '\0';
    char clean_pseudo[MAX_PSEUDO_LEN];
    if (sscanf(my_pseudo, "%s", clean_pseudo) != 1) {
        fprintf(stderr, "%s[ERREUR] Le pseudo ne doit pas être vide.%s\n", COLOR_RED, COLOR_RESET);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    strcpy(my_pseudo, clean_pseudo);
    
    // Envoyer la demande d'authentification
    ChatMessage connect_msg;
    connect_msg.type = MSG_CONNECT;
    strcpy(connect_msg.sender, my_pseudo);
    connect_msg.status = 0;
    
    if (send_all(sock_fd, &connect_msg, sizeof(ChatMessage)) < 0) {
        fprintf(stderr, "%s[ERREUR] Échec de l'envoi du pseudo.%s\n", COLOR_RED, COLOR_RESET);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    // Attendre la réponse de validation du serveur
    ChatMessage response;
    if (recv_all(sock_fd, &response, sizeof(ChatMessage)) <= 0) {
        fprintf(stderr, "%s[ERREUR] Pas de réponse du serveur.%s\n", COLOR_RED, COLOR_RESET);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    if (response.status != 0) {
        fprintf(stderr, "%s[ERREUR] %s%s\n", COLOR_RED, response.content, COLOR_RESET);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    // Affichage premium de connexion réussie
    printf("%s[INFO] %s%s\n", COLOR_GREEN, response.content, COLOR_RESET);
    printf("--- Astuce : tapez /users pour lister les participants, /msg <pseudo> <message> pour un MP, /quit pour quitter ---\n");
    draw_prompt(my_pseudo);
    
    fd_set read_fds;
    char input_buf[MAX_CONTENT_LEN + 128];
    
    // Boucle asynchrone principale du client
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock_fd, &read_fds);
        
        int activity = select(sock_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Erreur select");
            break;
        }
        
        // Cas 1 : Données reçues du serveur
        if (FD_ISSET(sock_fd, &read_fds)) {
            ChatMessage incoming;
            int nbytes = recv_all(sock_fd, &incoming, sizeof(ChatMessage));
            if (nbytes <= 0) {
                clear_line();
                printf("%s[INFO] Connexion fermée par le serveur. Déconnexion...%s\n", COLOR_RED, COLOR_RESET);
                break;
            }
            
            // Effacer la ligne de saisie actuelle pour afficher le message proprement
            clear_line();
            
            switch (incoming.type) {
                case MSG_NOTIFICATION:
                    printf("%s[Système] %s%s\n", COLOR_YELLOW, incoming.content, COLOR_RESET);
                    break;
                    
                case MSG_PUBLIC:
                    printf("%s%s%s : %s\n", COLOR_GREEN, incoming.sender, COLOR_RESET, incoming.content);
                    break;
                    
                case MSG_PRIVATE:
                    printf("%s%s (message privé reçu) : \"%s\"%s\n", COLOR_MAGENTA, incoming.sender, incoming.content, COLOR_RESET);
                    break;
                    
                case MSG_USERS:
                    printf("%s%s%s", COLOR_GREEN, incoming.content, COLOR_RESET);
                    break;
                    
                default:
                    break;
            }
            
            // Redessiner le prompt pour que l'utilisateur sache qu'il peut taper
            draw_prompt(my_pseudo);
        }
        
        // Cas 2 : L'utilisateur tape un message sur l'entrée standard
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
                break;
            }
            
            // Supprimer le saut de ligne final
            input_buf[strcspn(input_buf, "\n")] = '\0';
            
            if (strlen(input_buf) == 0) {
                draw_prompt(my_pseudo);
                continue;
            }
            
            ChatMessage out_msg;
            memset(&out_msg, 0, sizeof(ChatMessage));
            strcpy(out_msg.sender, my_pseudo);
            
            if (strncmp(input_buf, "/quit", 5) == 0) {
                out_msg.type = MSG_DISCONNECT;
                send_all(sock_fd, &out_msg, sizeof(ChatMessage));
                printf("Déconnexion...\n");
                break;
            } 
            else if (strncmp(input_buf, "/users", 6) == 0) {
                out_msg.type = MSG_USERS;
                send_all(sock_fd, &out_msg, sizeof(ChatMessage));
            } 
            else if (strncmp(input_buf, "/msg ", 5) == 0) {
                char target[MAX_PSEUDO_LEN];
                char text[MAX_CONTENT_LEN];
                
                // Extraction du pseudo destinataire et du contenu du message
                if (sscanf(input_buf + 5, "%s %[^\n]", target, text) == 2) {
                    out_msg.type = MSG_PRIVATE;
                    strcpy(out_msg.recipient, target);
                    strcpy(out_msg.content, text);
                    
                    send_all(sock_fd, &out_msg, sizeof(ChatMessage));
                    
                    // Écho local de confirmation d'envoi privé
                    clear_line();
                    printf("%s[Privé pour %s] : %s%s\n", COLOR_MAGENTA, target, text, COLOR_RESET);
                    draw_prompt(my_pseudo);
                } else {
                    printf("%s[INFO] Syntaxe incorrecte. Exemple : /msg <pseudo> <message>%s\n", COLOR_YELLOW, COLOR_RESET);
                    draw_prompt(my_pseudo);
                }
            } 
            else {
                // Message public par défaut
                out_msg.type = MSG_PUBLIC;
                strcpy(out_msg.content, input_buf);
                
                send_all(sock_fd, &out_msg, sizeof(ChatMessage));
                
                // Écho local immédiat pour le message public envoyé
                clear_line();
                printf("%s%s (Vous)%s : %s\n", COLOR_CYAN, my_pseudo, COLOR_RESET, input_buf);
                draw_prompt(my_pseudo);
            }
        }
    }
    
    close(sock_fd);
    printf("Au revoir !\n");
    return 0;
}
