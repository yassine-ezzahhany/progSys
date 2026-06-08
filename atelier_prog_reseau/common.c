#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// Envoie exactement 'len' octets sur la socket 'socket_fd'
// Gère le cas où send() effectue un envoi partiel
int send_all(int socket_fd, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = buf;
    while (total < len) {
        ssize_t n = send(socket_fd, p + total, len - total, 0);
        if (n <= 0) {
            return -1; // Erreur ou déconnexion
        }
        total += n;
    }
    return 0; // Succès
}

// Lit exactement 'len' octets depuis la socket 'socket_fd'
// Gère le cas où recv() effectue une lecture partielle en raison de la fragmentation TCP
int recv_all(int socket_fd, void *buf, size_t len) {
    size_t total = 0;
    char *p = buf;
    while (total < len) {
        ssize_t n = recv(socket_fd, p + total, len - total, 0);
        if (n < 0) {
            return -1; // Erreur réseau
        }
        if (n == 0) {
            return 0; // La socket distante a été fermée (déconnexion)
        }
        total += n;
    }
    return (int)total; // Succès (retourne le nombre d'octets lus, égal à len)
}
