/*
 * chatd_udp.c
 *
 * Compilation :
 * gcc chatd_udp.c -o chatd_udp
 *
 * Exécution :
 * ./chatd_udp 5000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define TAILLE_BUFFER 256

void CHATS(int D, struct sockaddr_in *client, socklen_t taille);

int main(int argc, char *argv[])
{
    int desc;
    int err;

    char buffer[TAILLE_BUFFER];

    struct sockaddr_in addrl;
    struct sockaddr_in addr_distant;

    socklen_t taille_addr = sizeof(addr_distant);

    if (argc != 2)
    {
        printf("Usage : %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Création socket UDP */
    desc = socket(AF_INET, SOCK_DGRAM, 0);

    if (desc < 0)
    {
        perror("Erreur socket");
        exit(EXIT_FAILURE);
    }

    memset(&addrl, 0, sizeof(addrl));

    addrl.sin_family = AF_INET;
    addrl.sin_addr.s_addr = INADDR_ANY;
    addrl.sin_port = htons(atoi(argv[1]));

    err = bind(desc,
               (struct sockaddr *)&addrl,
               sizeof(addrl));

    if (err != 0)
    {
        perror("Erreur bind");
        close(desc);
        exit(EXIT_FAILURE);
    }

    printf("Serveur UDP en attente...\n");

    /* Premier message pour connaître le client */
    recvfrom(desc,
             buffer,
             sizeof(buffer),
             0,
             (struct sockaddr *)&addr_distant,
             &taille_addr);

    printf("Client detecte : %s:%d\n",
           inet_ntoa(addr_distant.sin_addr),
           ntohs(addr_distant.sin_port));

    printf("Premier message : %s\n", buffer);

    CHATS(desc, &addr_distant, taille_addr);

    close(desc);

    return 0;
}

void CHATS(int D,
           struct sockaddr_in *client,
           socklen_t taille)
{
    char psl[50];
    char psd[50];

    char me[TAILLE_BUFFER];
    char mr[TAILLE_BUFFER];

    printf("Votre pseudo : ");
    fgets(psl, sizeof(psl), stdin);
    psl[strcspn(psl, "\n")] = '\0';

    sendto(D,
           psl,
           strlen(psl) + 1,
           0,
           (struct sockaddr *)client,
           taille);

    recvfrom(D,
             psd,
             sizeof(psd),
             0,
             NULL,
             NULL);

    printf("Pseudo distant : %s\n", psd);

    do
    {
        printf("%s : ", psl);

        fgets(me, sizeof(me), stdin);
        me[strcspn(me, "\n")] = '\0';

        sendto(D,
               me,
               strlen(me) + 1,
               0,
               (struct sockaddr *)client,
               taille);

        if (strcmp(me, "quitter") == 0)
            break;

        recvfrom(D,
                 mr,
                 sizeof(mr),
                 0,
                 NULL,
                 NULL);

        printf("%s : %s\n", psd, mr);

    } while (strcmp(mr, "quitter") != 0);
}