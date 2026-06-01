#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

int main(int argc, char *argv[]) {
    // 1. Vérification des arguments passés en ligne de commande
    if (argc < 2) {
        printf("=======================================================================\n");
        printf("          Recherche d'utilisateur par UID (Exercice 6)                 \n");
        printf("=======================================================================\n");
        printf("[Usage] %s <UID>\n", argv[0]);
        printf("Exemple : %s 1000\n", argv[0]);
        printf("=======================================================================\n");
        return 1;
    }

    // 2. Conversion de la chaîne de caractères argv[1] en entier à l'aide d'atoi
    int uid_int = atoi(argv[1]);
    uid_t uid = (uid_t)uid_int;

    // 3. Recherche de l'utilisateur associé à cet UID
    //    getpwuid() retourne un pointeur vers une structure passwd
    struct passwd *pw = getpwuid(uid);

    if (pw == NULL) {
        // Contrainte explicite de l'énoncé si l'UID n'existe pas :
        printf("il n'y a pas d'utilisateur associé à cet uid.\n");
        return 0;
    }

    // 4. Si l'utilisateur est trouvé, on affiche ses détails
    printf("=======================================================================\n");
    printf("     Informations d'identité pour l'UID %d (Trouvé !)                  \n", uid_int);
    printf("=======================================================================\n");
    printf("- Nom d'utilisateur (Login)    : %s\n", pw->pw_name);
    printf("- Nom complet (GECOS)          : %s\n", pw->pw_gecos);
    printf("- Répertoire de base (Home)    : %s\n", pw->pw_dir);
    printf("- Shell par défaut             : %s\n", pw->pw_shell);

    // 5. Utilisation de getgrgid et de la structure group pour afficher le groupe principal en clair
    //    pw->pw_gid contient l'ID du groupe principal de l'utilisateur
    struct group *gr = getgrgid(pw->pw_gid);
    if (gr != NULL) {
        printf("- Groupe principal             : %s (GID: %d)\n", gr->gr_name, pw->pw_gid);
    } else {
        printf("- Groupe principal             : inconnu (GID: %d)\n", pw->pw_gid);
    }
    printf("=======================================================================\n");

    return 0;
}
