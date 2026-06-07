/*
 * chatc_udp.c
 *
 * Compilation :
 * gcc chatc_udp.c -o chatc_udp
 *
 * Exécution :
 * ./chatc_udp 127.0.0.1 5000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define TAILLE_BUFFER 256

void CHATC(int D,
           struct sockaddr_in *serveur,
           socklen_t taille);

int main(int argc, char *argv[])
{
    int desc;

    struct sockaddr_in addr_distant;

    if (argc != 3)
    {
        printf("Usage : %s ip port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    desc = socket(AF_INET,
                  SOCK_DGRAM,
                  0);

    if (desc < 0)
    {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr_distant, 0, sizeof(addr_distant));

    addr_distant.sin_family = AF_INET;
    addr_distant.sin_addr.s_addr = inet_addr(argv[1]);
    addr_distant.sin_port = htons(atoi(argv[2]));

    /* Message initial */
    sendto(desc,
           "HELLO",
           6,
           0,
           (struct sockaddr *)&addr_distant,
           sizeof(addr_distant));

    printf("Serveur contacte.\n");

    CHATC(desc,
          &addr_distant,
          sizeof(addr_distant));

    close(desc);

    return 0;
}

void CHATC(int D,
           struct sockaddr_in *serveur,
           socklen_t taille)
{
    char psl[50];
    char psd[50];

    char me[TAILLE_BUFFER];
    char mr[TAILLE_BUFFER];

    recvfrom(D,
             psd,
             sizeof(psd),
             0,
             NULL,
             NULL);

    printf("Pseudo distant : %s\n", psd);

    printf("Votre pseudo : ");

    fgets(psl, sizeof(psl), stdin);

    psl[strcspn(psl, "\n")] = '\0';

    sendto(D,
           psl,
           strlen(psl) + 1,
           0,
           (struct sockaddr *)serveur,
           taille);

    do
    {
        recvfrom(D,
                 mr,
                 sizeof(mr),
                 0,
                 NULL,
                 NULL);

        printf("%s : %s\n", psd, mr);

        if (strcmp(mr, "quitter") == 0)
            break;

        printf("%s : ", psl);

        fgets(me, sizeof(me), stdin);

        me[strcspn(me, "\n")] = '\0';

        sendto(D,
               me,
               strlen(me) + 1,
               0,
               (struct sockaddr *)serveur,
               taille);

    } while (strcmp(me, "quitter") != 0);
}