#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid_ls, pid_ps;

    printf("=======================================================================\n");
    printf("     Gestion et synchronisation de processus (Exercice 5)              \n");
    printf("=======================================================================\n");
    printf("[Père - PID: %d] Lancement des deux processus fils...\n\n", getpid());

    // 1. Création du premier processus fils pour exécuter 'ls -l'
    pid_ls = fork();
    if (pid_ls < 0) {
        perror("Erreur fork (ls)");
        return 1;
    } 
    else if (pid_ls == 0) {
        // Dans le premier fils
        printf("[Fils LS - PID: %d] Exécution de 'ls -l' :\n", getpid());
        execlp("ls", "ls", "-l", NULL);
        
        // Si execlp échoue
        perror("Erreur execlp de ls");
        exit(EXIT_FAILURE);
    }

    // 2. Création du second processus fils pour exécuter 'ps -l'
    pid_ps = fork();
    if (pid_ps < 0) {
        perror("Erreur fork (ps)");
        return 1;
    } 
    else if (pid_ps == 0) {
        // Dans le second fils
        // Nous ajoutons un léger temps de sommeil si nécessaire, mais en général la vitesse dépend de l'OS.
        printf("[Fils PS - PID: %d] Exécution de 'ps -l' :\n", getpid());
        execlp("ps", "ps", "-l", NULL);
        
        // Si execlp échoue
        perror("Erreur execlp de ps");
        exit(EXIT_FAILURE);
    }

    // 3. Code du père : attente des fils
    int status;
    
    // Le premier wait() intercepte le PREMIER fils qui se termine.
    // wait() bloque le père jusqu'à ce qu'un de ses fils se termine et renvoie son PID.
    pid_t premier_termine = wait(&status);

    if (premier_termine < 0) {
        perror("Erreur wait");
        return 1;
    }

    printf("\n=======================================================================\n");
    printf("                       SYNCHRONISATION ET RÉSULTAT                     \n");
    printf("=======================================================================\n");
    
    if (premier_termine == pid_ls) {
        printf("[Résultat] Le PREMIER fils à s'être terminé est : Fils 1 (Commande 'ls -l') [PID: %d]\n", premier_termine);
    } 
    else if (premier_termine == pid_ps) {
        printf("[Résultat] Le PREMIER fils à s'être terminé est : Fils 2 (Commande 'ps -l') [PID: %d]\n", premier_termine);
    } 
    else {
        printf("[Résultat] Processus inattendu terminé [PID: %d]\n", premier_termine);
    }

    // 4. Attente obligatoire du second fils restant pour éviter de laisser un processus zombie
    pid_t second_termine = wait(&status);
    if (second_termine > 0) {
        printf("[Père] Le second fils [PID: %d] s'est également terminé.\n", second_termine);
    }

    printf("[Père] Fin de l'attente générale. Terminaison du programme père.\n");
    printf("=======================================================================\n");

    return 0;
}
