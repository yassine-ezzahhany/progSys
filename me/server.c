/*
 * chatd.c
 * Serveur TCP de discussion
 *
 * Compilation :
 * gcc chatd.c -o chatd
 *
 * Exécution :
 * ./chatd 5000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define NBRMAX 5
#define TAILLE_BUFFER 256

void CHATS(int D);

int main(int argc, char *argv[])
{
    int desc_ecoute;
    int desc_client;
    int err;

    struct sockaddr_in addrl;
    struct sockaddr_in addr_distant;

    socklen_t taille_addr = sizeof(addr_distant);

    /* Création du socket TCP */
    desc_ecoute = socket(AF_INET, SOCK_STREAM, 0);

    if (desc_ecoute < 0)
    {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    /* Vérification du numéro de port */
    if (argc != 2)
    {
        printf("Usage : %s numPort\n", argv[0]);
        close(desc_ecoute);
        exit(EXIT_FAILURE);
    }

    /* Préparation de l'adresse locale */
    memset(&addrl, 0, sizeof(addrl));

    addrl.sin_family = AF_INET;
    addrl.sin_addr.s_addr = INADDR_ANY;
    addrl.sin_port = htons(atoi(argv[1]));

    /* Bind */
    err = bind(desc_ecoute,
               (struct sockaddr *)&addrl,
               sizeof(addrl));

    if (err != 0)
    {
        perror("Erreur bind");
        close(desc_ecoute);
        exit(EXIT_FAILURE);
    }

    /* Mise en écoute */
    err = listen(desc_ecoute, NBRMAX);

    if (err != 0)
    {
        perror("Erreur listen");
        close(desc_ecoute);
        exit(EXIT_FAILURE);
    }

    printf("Serveur en attente sur le port %s...\n", argv[1]);

    while (1)
    {
        /* Acceptation d'un client */
        desc_client = accept(desc_ecoute,
                             (struct sockaddr *)&addr_distant,
                             &taille_addr);

        if (desc_client > 0)
        {
            printf("\nClient connecté\n");
            printf("IP   : %s\n",
                   inet_ntoa(addr_distant.sin_addr));

            printf("Port : %d\n",
                   ntohs(addr_distant.sin_port));

            CHATS(desc_client);
        }
    }

    close(desc_ecoute);

    return 0;
}

/* Fonction de discussion */
void CHATS(int D)
{
    char psl[50];
    char psd[50];

    char me[TAILLE_BUFFER];
    char mr[TAILLE_BUFFER];

    memset(psl, 0, sizeof(psl));
    memset(psd, 0, sizeof(psd));

    printf("Saisir votre pseudo : ");
    fgets(psl, sizeof(psl), stdin);

    psl[strcspn(psl, "\n")] = '\0';

    /* Envoi du pseudo local */
    send(D, psl, strlen(psl) + 1, 0);

    /* Réception du pseudo distant */
    recv(D, psd, sizeof(psd), 0);

    printf("Pseudo distant : %s\n", psd);

    do
    {
        memset(me, 0, sizeof(me));
        memset(mr, 0, sizeof(mr));

        printf("%s : ", psl);

        fgets(me, sizeof(me), stdin);

        me[strcspn(me, "\n")] = '\0';

        send(D, me, strlen(me) + 1, 0);

        if (strcmp(me, "quitter") == 0)
            break;

        recv(D, mr, sizeof(mr), 0);

        printf("%s : %s\n", psd, mr);

    } while (strcmp(me, "quitter") != 0 &&
             strcmp(mr, "quitter") != 0);

    printf("Fin de la discussion.\n");

    close(D);
}