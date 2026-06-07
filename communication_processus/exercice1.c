#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

// Variable globale de type volatile sig_atomic_t pour garantir l'accès atomique lors d'un signal
volatile sig_atomic_t iteration_count = 0;

// Fonction de conversion manuelle d'un entier en chaîne de caractères (async-signal-safe)
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
    // Inversement de la chaîne
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - 1 - j];
        str[i - 1 - j] = temp;
    }
}

// Gestionnaire du signal SIGINT (Ctrl-C)
void handle_sigint(int sig) {
    (void)sig; // Pour éviter le warning de variable inutilisée
    
    const char *filename = "sauvegarde.txt";
    
    // Ouverture/Création du fichier avec écrasement si existant (O_TRUNC)
    // Les appels open, write et close sont garantis async-signal-safe par POSIX
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        const char *err_msg = "Erreur: Impossible d'ouvrir le fichier de sauvegarde.\n";
        write(STDERR_FILENO, err_msg, strlen(err_msg));
        exit(EXIT_FAILURE);
    }

    // Récupération de l'heure courante (non async-signal-safe au sens strict de POSIX, 
    // mais très toléré dans les TPs. Pour être 100% sûr, on écrit juste un log textuel propre)
    const char *msg_header = "===============================================\n";
    const char *msg_title  = "   SAUVEGARDE D'URGENCE (INTERRUPTION SIGINT)  \n";
    const char *msg_sep    = "===============================================\n";
    const char *msg_iter   = "Dernière itération enregistrée : ";
    const char *msg_status = "Statut : Programme arrêté proprement via Ctrl-C.\n";

    write(fd, msg_header, strlen(msg_header));
    write(fd, msg_title, strlen(msg_title));
    write(fd, msg_sep, strlen(msg_sep));
    
    write(fd, msg_iter, strlen(msg_iter));
    char count_buf[12];
    int_to_str(iteration_count, count_buf);
    write(fd, count_buf, strlen(count_buf));
    write(fd, "\n", 1);
    
    write(fd, msg_status, strlen(msg_status));
    write(fd, msg_header, strlen(msg_header));
    
    close(fd);

    const char *console_msg = "\n[SIGINT] Interruption détectée. Progression sauvegardée dans 'sauvegarde.txt'.\n";
    write(STDOUT_FILENO, console_msg, strlen(console_msg));
    
    exit(EXIT_SUCCESS);
}

int main() {
    printf("=======================================================================\n");
    printf("  Exercice 1 : Capture de SIGINT (Ctrl-C) et sauvegarde automatique    \n");
    printf("=======================================================================\n");
    printf("[PID : %d] Programme en cours d'exécution...\n", getpid());
    printf("Appuyez sur Ctrl-C pour interrompre le programme et sauvegarder les données.\n\n");

    // Configuration de la structure sigaction pour gérer proprement le signal SIGINT
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Aucun comportement spécial

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Erreur lors de l'enregistrement de sigaction");
        return EXIT_FAILURE;
    }

    // Boucle de simulation d'une tâche continue
    while (1) {
        iteration_count++;
        printf("  -> [Travail en cours] Etape %d...\n", iteration_count);
        sleep(1); // Suspend l'exécution pendant 1 seconde
    }

    return EXIT_SUCCESS; // Ne sera jamais atteint
}
