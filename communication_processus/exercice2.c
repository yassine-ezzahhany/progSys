#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(void)
{
    int tube[2];
    char * buffer = "AZERTYUIOP";

    fprintf(stdout, "Création tube \n");
    if (pipe(tube) != 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Fermeture du descripteur de lecture (tube[0]).
    // À partir de cet instant, le tube n'a plus aucun lecteur connecté.
    fprintf(stdout, "Fermeture sortie \n");
    close(tube[0]);
    
    // Tentative d'écriture dans le descripteur d'écriture (tube[1]).
    // L'écriture dans un tube sans lecteur actif provoque l'envoi immédiat 
    // par le noyau Unix du signal SIGPIPE (Signal 13) au processus écrivain.
    // Par défaut, la réception de ce signal termine brusquement le programme.
    fprintf(stdout, "Écriture dans tube \n");
    if (write(tube[1], buffer, strlen(buffer)) != (ssize_t)strlen(buffer)) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    
    // Cette ligne ne sera jamais exécutée car le processus sera tué par SIGPIPE avant.
    fprintf(stdout, "Fin du programme \n");
    return EXIT_SUCCESS;
}
