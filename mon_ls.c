#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <linux/limits.h>

void print_permissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : (S_ISLNK(mode)) ? "l" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void lister_repertoire(const char *chemin, int recursif, int format_long, int niveau) {
    DIR *dossier;
    struct dirent *entree;
    struct stat infos_fichier;
    char chemin_complet[PATH_MAX];

    dossier = opendir(chemin);
    if (dossier == NULL) {
        fprintf(stderr, "Error opening '%s': %s\n", chemin, strerror(errno));
        return;
    }

    while ((entree = readdir(dossier)) != NULL) {
        if (strcmp(entree->d_name, ".") == 0 || strcmp(entree->d_name, "..") == 0) {
            continue;
        }

        if (snprintf(chemin_complet, sizeof(chemin_complet), "%s/%s", chemin, entree->d_name) >= PATH_MAX) {
            continue;
        }

        if (lstat(chemin_complet, &infos_fichier) == 0) {
            for (int i = 0; i < niveau; i++) printf("    ");
            printf("|-- ");

            if (format_long) {
                print_permissions(infos_fichier.st_mode);

                struct passwd *pw = getpwuid(infos_fichier.st_uid);
                struct group  *gr = getgrgid(infos_fichier.st_gid);

                char time_str[256];
                struct tm *tm_info = localtime(&infos_fichier.st_mtime);
                strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);

                printf(" %lu", (unsigned long)infos_fichier.st_nlink);
                
                if (pw) printf(" %-8s", pw->pw_name);
                else    printf(" %-8u", infos_fichier.st_uid);

                if (gr) printf(" %-8s", gr->gr_name);
                else    printf(" %-8u", infos_fichier.st_gid);

                printf(" %8ld %s ", (long)infos_fichier.st_size, time_str);
            }

            printf("%s\n", entree->d_name);

            if (recursif && S_ISDIR(infos_fichier.st_mode)) {
                lister_repertoire(chemin_complet, recursif, format_long, niveau + 1);
            }
        }
    }

    closedir(dossier);
}

int main(int argc, char *argv[]) {
    int recursif = 0;
    int format_long = 0;
    const char *chemin_cible = ".";

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j] != '\0'; j++) {
                if (argv[i][j] == 'r') recursif = 1;
                else if (argv[i][j] == 'l') format_long = 1;
                else fprintf(stderr, "Unknown option: -%c\n", argv[i][j]);
            }
        } else {
            chemin_cible = argv[i];
        }
    }

    printf("Contents of: %s\n", chemin_cible);
    lister_repertoire(chemin_cible, recursif, format_long, 0);

    return EXIT_SUCCESS;
}