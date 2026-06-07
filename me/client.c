/*
 * chatc.c
 * Client TCP de discussion
 *
 * Compilation :
 * gcc chatc.c -o chatc
 *
 * Exécution :
 * ./chatc 127.0.0.1 5000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define TAILLE_BUFFER 256

void CHATC(int D);

int main(int argc, char *argv[])
{
    int desc;
    int err;

    struct sockaddr_in addr_distant;

    /* Vérification des arguments */
    if (argc != 3)
    {
        printf("Usage : %s adresseIP port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Création du socket TCP */
    desc = socket(AF_INET, SOCK_STREAM, 0);

    if (desc < 0)
    {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    /* Configuration de l'adresse du serveur */
    memset(&addr_distant, 0, sizeof(addr_distant));

    addr_distant.sin_family = AF_INET;
    addr_distant.sin_addr.s_addr = inet_addr(argv[1]);
    addr_distant.sin_port = htons(atoi(argv[2]));

    /* Connexion au serveur */
    err = connect(desc,
                  (struct sockaddr *)&addr_distant,
                  sizeof(addr_distant));

    if (err != 0)
    {
        perror("Erreur connect");
        close(desc);
        exit(EXIT_FAILURE);
    }

    printf("Connecte au serveur %s:%s\n",
           argv[1],
           argv[2]);

    CHATC(desc);

    return 0;
}

/* Fonction de discussion */
void CHATC(int D)
{
    char psl[50];
    char psd[50];

    char me[TAILLE_BUFFER];
    char mr[TAILLE_BUFFER];

    memset(psl, 0, sizeof(psl));
    memset(psd, 0, sizeof(psd));

    /* Réception du pseudo du serveur */
    recv(D, psd, sizeof(psd), 0);

    printf("Le pseudo distant est : %s\n", psd);

    /* Saisie du pseudo local */
    printf("Donner votre pseudo : ");

    fgets(psl, sizeof(psl), stdin);

    psl[strcspn(psl, "\n")] = '\0';

    /* Envoi du pseudo local */
    send(D, psl, strlen(psl) + 1, 0);

    do
    {
        memset(me, 0, sizeof(me));
        memset(mr, 0, sizeof(mr));

        /* Réception du message du serveur */
        recv(D, mr, sizeof(mr), 0);

        printf("%s : %s\n", psd, mr);

        if (strcmp(mr, "quitter") == 0)
            break;

        /* Saisie du message local */
        printf("%s : ", psl);

        fgets(me, sizeof(me), stdin);

        me[strcspn(me, "\n")] = '\0';

        /* Envoi */
        send(D, me, strlen(me) + 1, 0);

    }
    while (strcmp(me, "quitter") != 0);

    printf("Fin de la discussion.\n");

    close(D);
}