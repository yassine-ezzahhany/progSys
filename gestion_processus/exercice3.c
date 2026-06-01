#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    const char *filename = "testFile.txt";
    // Texte initial demandé
    const char *initial_text = "le noyau réalise une préemption du processeur lorsqu'il dépasse son temps.";
    const char *insert_text = "quantum de ";
    const char *target = "son temps";

    printf("=======================================================================\n");
    printf("     Manipulation de fichier de bas niveau (Exercice 3)                \n");
    printf("=======================================================================\n");

    // 1. Ouverture du fichier en Lecture/Écriture. Il est créé s'il n'existe pas,
    //    et tronqué à 0 s'il existe déjà. Droits d'accès : 0644 (rw-r--r--)
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Erreur open (initialisation)");
        return 1;
    }
    printf("[1] Fichier '%s' ouvert avec succès (fd=%d).\n", filename, fd);

    // 2. Écriture du texte initial
    ssize_t bytes_written = write(fd, initial_text, strlen(initial_text));
    if (bytes_written < 0) {
        perror("Erreur write (écriture initiale)");
        close(fd);
        return 1;
    }
    printf("[2] Écriture du texte initial réussie (%ld octets).\n", (long)bytes_written);

    // 3. Lecture du fichier pour identifier de façon sûre l'emplacement de "son temps"
    //    On se replace au début du fichier avec lseek
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Erreur lseek (retour au début)");
        close(fd);
        return 1;
    }

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Erreur read");
        close(fd);
        return 1;
    }

    // Recherche de l'emplacement de la chaîne cible "son temps"
    char *pos = strstr(buffer, target);
    if (!pos) {
        fprintf(stderr, "Erreur : La chaîne '%s' n'a pas été trouvée dans le texte.\n", target);
        close(fd);
        return 1;
    }

    // Calcul de l'offset en octets par rapport au début du fichier
    off_t offset = pos - buffer;
    printf("[3] Chaîne cible '%s' repérée à l'offset (position) : %ld octets.\n", target, (long)offset);

    // 4. Sauvegarde de la fin du texte ("son temps.") pour ne pas l'écraser irrémédiablement
    char saved_suffix[128];
    strcpy(saved_suffix, pos);
    printf("    -> Portion de fin sauvegardée : \"%s\"\n", saved_suffix);

    // 5. Positionnement du pointeur d'écriture juste avant "son temps" grâce à lseek()
    if (lseek(fd, offset, SEEK_SET) < 0) {
        perror("Erreur lseek (recherche offset cible)");
        close(fd);
        return 1;
    }
    printf("[4] Pointeur déplacé à la position %ld.\n", (long)offset);

    // 6. Écriture de "quantum de " à cette position
    bytes_written = write(fd, insert_text, strlen(insert_text));
    if (bytes_written < 0) {
        perror("Erreur write (insertion de 'quantum de ')");
        close(fd);
        return 1;
    }
    printf("[5] Insertion de la chaîne '%s' effectuée.\n", insert_text);

    // 7. Ré-écriture immédiate du suffixe sauvegardé ("son temps.") à la suite
    bytes_written = write(fd, saved_suffix, strlen(saved_suffix));
    if (bytes_written < 0) {
        perror("Erreur write (réécriture de la fin du texte)");
        close(fd);
        return 1;
    }
    printf("[6] Fin du texte réécrite avec succès.\n");

    // 8. Lecture finale de l'ensemble du fichier pour vérifier le résultat
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Erreur lseek (repositionnement final)");
        close(fd);
        return 1;
    }

    memset(buffer, 0, sizeof(buffer));
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        perror("Erreur read final");
        close(fd);
        return 1;
    }

    printf("\n[Résultat] Contenu final du fichier lu depuis le disque :\n\"%s\"\n", buffer);

    // 9. Fermeture du descripteur de fichier
    close(fd);
    printf("\n[7] Fichier clos avec succès. Fin du programme.\n");

    return 0;
}
