#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    // Déclarations des deux tubes
    // p1 : du père vers le fils (père écrit dans p1[1], fils lit dans p1[0])
    // p2 : du fils vers le père (fils écrit dans p2[1], père lit dans p2[0])
    int p1[2];
    int p2[2];

    printf("=======================================================================\n");
    printf("  Exercice 3 : Communication bidirectionnelle via tubes (Père <-> Fils) \n");
    printf("=======================================================================\n");

    // Création des deux tubes
    if (pipe(p1) != 0) {
        perror("Erreur création tube p1");
        return EXIT_FAILURE;
    }
    if (pipe(p2) != 0) {
        perror("Erreur création tube p2");
        close(p1[0]);
        close(p1[1]);
        return EXIT_FAILURE;
    }

    printf("[Père] Tubes p1 et p2 initialisés avec succès.\n");

    // Création du processus fils
    pid_t pid = fork();

    if (pid < 0) {
        perror("Erreur lors du fork");
        close(p1[0]); close(p1[1]);
        close(p2[0]); close(p2[1]);
        return EXIT_FAILURE;
    }
    else if (pid == 0) {
        // ==========================================
        //               CODE DU FILS
        // ==========================================
        // Fermeture des descripteurs inutiles :
        // Le fils lit dans p1, donc ferme l'écriture de p1.
        close(p1[1]);
        // Le fils écrit dans p2, donc ferme la lecture de p2.
        close(p2[0]);

        printf("[Fils - PID: %d] Prêt à recevoir les entiers sur le tube p1...\n", getpid());

        int recu;
        int double_val;

        // Boucle pour recevoir 5 entiers
        for (int i = 0; i < 5; i++) {
            // Lecture sur p1[0]
            if (read(p1[0], &recu, sizeof(int)) <= 0) {
                perror("[Fils] Erreur lecture sur p1");
                close(p1[0]);
                close(p2[1]);
                exit(EXIT_FAILURE);
            }

            printf("[Fils - PID: %d] Entier %d reçu : %d\n", getpid(), i + 1, recu);

            // Calcul du double
            double_val = recu * 2;

            // Écriture du double sur p2[1]
            if (write(p2[1], &double_val, sizeof(int)) <= 0) {
                perror("[Fils] Erreur écriture sur p2");
                close(p1[0]);
                close(p2[1]);
                exit(EXIT_FAILURE);
            }
            usleep(10000); // Micro-pause pour la synchronisation
        }

        // Fermeture finale des descripteurs
        close(p1[0]);
        close(p2[1]);
        printf("[Fils - PID: %d] Terminé. Libération des ressources.\n", getpid());
        exit(EXIT_SUCCESS);
    }
    else {
        // ==========================================
        //               CODE DU PÈRE
        // ==========================================
        // Fermeture des descripteurs inutiles :
        // Le père écrit dans p1, donc ferme la lecture de p1.
        close(p1[0]);
        // Le père lit dans p2, donc ferme l'écriture de p2.
        close(p2[1]);

        // Tableau des 5 entiers à transmettre
        int entiers_a_envoyer[5] = {5, 12, 23, 42, 99};
        int doubles_recus[5];

        printf("[Père - PID: %d] Début de l'envoi des 5 entiers au fils...\n\n", getpid());

        for (int i = 0; i < 5; i++) {
            printf("[Père - PID: %d] Envoi de l'entier %d : %d\n", getpid(), i + 1, entiers_a_envoyer[i]);

            // Écriture sur p1[1]
            if (write(p1[1], &entiers_a_envoyer[i], sizeof(int)) <= 0) {
                perror("[Père] Erreur écriture sur p1");
                close(p1[1]);
                close(p2[0]);
                return EXIT_FAILURE;
            }

            // Lecture sur p2[0] pour obtenir le double
            if (read(p2[0], &doubles_recus[i], sizeof(int)) <= 0) {
                perror("[Père] Erreur lecture sur p2");
                close(p1[1]);
                close(p2[0]);
                return EXIT_FAILURE;
            }

            printf("[Père - PID: %d] Double reçu du fils : %d\n\n", getpid(), doubles_recus[i]);
        }

        // Attente de la terminaison propre du fils
        int status;
        wait(&status);

        // Fermeture finale des descripteurs
        close(p1[1]);
        close(p2[0]);

        printf("=======================================================================\n");
        printf("[Père - PID: %d] Résumé des résultats :\n", getpid());
        for (int i = 0; i < 5; i++) {
            printf("  -> Valeur initiale: %2d  |  Double calculé par le fils: %2d\n", 
                   entiers_a_envoyer[i], doubles_recus[i]);
        }
        printf("=======================================================================\n");
        printf("[Père] Succès de la transmission. Fin du programme.\n");
    }

    return EXIT_SUCCESS;
}
