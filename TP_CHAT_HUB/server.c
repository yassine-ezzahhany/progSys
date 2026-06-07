#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

void error(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

void shats(int fd);

int main(int argc, char *argv[])
{

  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  // VARIABLES DECLARATIONS
  int port, socket_fd, new_socket_fd;
  const char *hostname;
  socklen_t clilen;
  struct sockaddr_in server_addr, client_addr;
  // VARIABLES PARSING
  hostname = argv[1];
  port = atoi(argv[2]);
  // CREATING THE socket
  socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (socket_fd < 0)
  {
    error("ERROR on socket creation");
  }

  bzero((char *)&server_addr, sizeof(server_addr));
  // SET THE TYPE AS IPV4
  server_addr.sin_family = AF_INET;
  // ACCEPT INCOMING CONNECTIONS FROM ANY INTERFACE ON PC
  // server_addr.sin_addr.s_addr = INADDR_ANY;
  // CONVERT STRING REPRESENTATION TO ITS BINARY FORMAT
  if(inet_pton(AF_INET, hostname, &server_addr.sin_addr) < 0) {
    error("ERROR invalid or unsupported format");
  }
  // TRANSLATE FROM FORMAT FROM LITTLE INDIAN TO BIG INDIAN
  server_addr.sin_port = htons(port);

  // WE PROVIDE THE SOCKET WITH INFO HE WILL USE TO ACCEPT INCOMING CONNECTIONS
  if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    error("ERROR on binding");
  //LISTEN TO INCOMING CONNECTIONS WITH A QUEUE OF ONE CLIENT
  printf("listening to incoming connection...\n");
  listen(socket_fd, 1);

  while (1) {
    clilen = sizeof(client_addr);
    new_socket_fd = accept(socket_fd,
                           (struct sockaddr *)&client_addr,
                           &clilen);
    if (new_socket_fd < 0) {
      error("ERROR on accept");
    }

    printf(
      "Client has connected with <IP> <%s> and PORT <%d>\n",
      inet_ntoa(client_addr.sin_addr),
      ntohs(client_addr.sin_port)
    );

    shats(new_socket_fd);
    close(new_socket_fd);
    printf("Waiting for next client...\n");
  }

  close(socket_fd);
  return EXIT_SUCCESS;
}

void shats(int fd)
{
  char buffer[256];
  int n;

  while (1)
  {
    bzero(buffer, 256);
    n = read(fd, buffer, 255);
    if (n < 0) {
      error("ERROR reading from socket");
    }
    else if (n == 0) {
      printf("Client has exited. shutting down...\n");
      break;
    }
    printf("Client : %s", buffer);
    bzero(buffer, 256);
    printf("Server : ");

    if (fgets(buffer, 255, stdin) == NULL) {
      printf("exiting...\n");
      break;
    }

    if (strncmp(buffer, "exit", 4) == 0) {
      printf("exiting...\n");
      break;
    }
    n = write(fd, buffer, strlen(buffer));
    if (n < 0)
      error("ERROR writing to socket");
  }
  printf("bye\n");
}
