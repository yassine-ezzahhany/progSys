#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    int n = 10; // Valeur par défaut
    
    // Si un argument est passé, on l'utilise pour N
    if (argc > 1) {
        n = atoi(argv[1]);
        if (n <= 0) {
            fprintf(stderr, "Veuillez entrer un nombre N strictement positif.\n");
            return 1;
        }
    } else {
        printf("[Information] Aucun argument fourni pour N. Utilisation de la valeur par défaut : %d\n", n);
        printf("[Usage] %s <N>\n\n", argv[0]);
    }

    printf("[Début] Lancement du programme (PID principal: %d) avec N = %d\n", getpid(), n);

    pid_t pid = fork();

    if (pid < 0) {
        // En cas d'erreur lors du fork
        perror("Erreur fork");
        return 1;
    } 
    else if (pid == 0) {
        // Code du processus fils
        printf("\n[Fils - PID: %d, Père PID: %d] Début de l'affichage de tous les nombres de 1 à %d :\n", getpid(), getppid(), n);
        for (int i = 1; i <= n; i++) {
            printf("  -> [Fils - PID: %d] Nombre : %d\n", getpid(), i);
            usleep(10000); // Petite pause pour éviter le chevauchement chaotique des sorties
        }
        printf("[Fils - PID: %d] *** Travail terminé, fin du processus fils. ***\n\n", getpid());
        exit(EXIT_SUCCESS);
    } 
    else {
        // Code du processus père
        printf("\n[Père - PID: %d, Fils PID: %d] Début de l'affichage des nombres pairs entre 1 à %d :\n", getpid(), pid, n);
        for (int i = 1; i <= n; i++) {
            if (i % 2 == 0) {
                printf("  -> [Père - PID: %d] Nombre Pair : %d\n", getpid(), i);
                usleep(10000);
            }
        }
        
        // Le père attend la fin du fils pour éviter les processus zombies
        int status;
        wait(&status);
        
        printf("[Père - PID: %d] *** Mon fils s'est terminé, fin du processus père. ***\n", getpid());
    }

    return 0;
}
