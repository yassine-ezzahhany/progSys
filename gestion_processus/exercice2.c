#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Fonction récursive modélisant l'arborescence des processus
void executer_processus(int id) {
    int nb_fils = 0;
    int ids_fils[3];

    // Définition de l'arborescence pour obtenir exactement 15 processus :
    // - P0 (Initial) -> 3 fils (P1, P2, P3)
    // - Tous les fils de P0 ont eux-mêmes des fils :
    //   - P1 -> 2 fils (P4, P5)
    //   - P2 -> 2 fils (P6, P7)
    //   - P3 -> 2 fils (P8, P9)
    // - Niveau supérieur (pour atteindre exactement 15) :
    //   - P4 -> 2 fils (P10, P11)
    //   - P5 -> 3 fils (P12, P13, P14)
    // - Tous les autres processus (P6 à P14) n'ont pas de fils (0 fils)
    if (id == 0) {
        nb_fils = 3;
        ids_fils[0] = 1; ids_fils[1] = 2; ids_fils[2] = 3;
    } else if (id == 1) {
        nb_fils = 2;
        ids_fils[0] = 4; ids_fils[1] = 5;
    } else if (id == 2) {
        nb_fils = 2;
        ids_fils[0] = 6; ids_fils[1] = 7;
    } else if (id == 3) {
        nb_fils = 2;
        ids_fils[0] = 8; ids_fils[1] = 9;
    } else if (id == 4) {
        nb_fils = 2;
        ids_fils[0] = 10; ids_fils[1] = 11;
    } else if (id == 5) {
        nb_fils = 3;
        ids_fils[0] = 12; ids_fils[1] = 13; ids_fils[2] = 14;
    }

    // Affichage des informations sur le processus en cours de création / d'exécution
    printf("Processus ID-Logique: %2d | mon PID: %5d | mon Père: %5d | va créer %d fils\n",
           id, getpid(), getppid(), nb_fils);

    // Boucle de fork() pour engendrer les fils nécessaires
    for (int i = 0; i < nb_fils; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Erreur fork");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) {
            // Le processus fils exécute sa portion d'arbre récursivement
            executer_processus(ids_fils[i]);
            
            // Une fois que l'arbre sous ce fils est complètement traité, il se termine proprement
            exit(EXIT_SUCCESS); 
        }
    }

    // Le processus père attend la fin de TOUS ses fils pour éviter les processus zombies
    for (int i = 0; i < nb_fils; i++) {
        wait(NULL);
    }

    // Message facultatif pour montrer le repli propre de la pile de processus
    printf("Processus ID-Logique: %2d | PID: %5d se termine proprement.\n", id, getpid());
}

int main() {
    printf("=======================================================================\n");
    printf("  Génération de l'arborescence de 15 processus (Contraintes respectées) \n");
    printf("=======================================================================\n");

    // Lancement de l'arborescence à partir du processus racine (ID 0)
    executer_processus(0);

    printf("\n[Succès] Processus initial (ID 0) a attendu tous ses descendants. Fin du programme.\n");
    return 0;
}
