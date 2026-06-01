#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

int main() {
    // 1. Ouverture du répertoire courant "."
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("Erreur opendir (impossible d'ouvrir le dossier courant)");
        return 1;
    }

    printf("=======================================================================\n");
    printf("     Affichage du répertoire courant - Équivalent de 'ls -ai'          \n");
    printf("=======================================================================\n");

    struct dirent *entry;
    int count = 0;

    // 2. Parcours de toutes les entrées du répertoire
    //    readdir() retourne automatiquement les fichiers cachés (commençant par '.')
    //    tels que "." (répertoire courant) et ".." (répertoire parent)
    while ((entry = readdir(dir)) != NULL) {
        // En Unix, l'inode est accessible via la structure dirent (d_ino)
        // d_name contient le nom en clair de l'entrée
        printf("%10lu %s\n", (unsigned long)entry->d_ino, entry->d_name);
        count++;
    }

    // 3. Fermeture du répertoire
    closedir(dir);

    printf("=======================================================================\n");
    printf("[Succès] Fin du parcours. Nombre total d'éléments trouvés : %d\n", count);
    printf("=======================================================================\n");

    return 0;
}
