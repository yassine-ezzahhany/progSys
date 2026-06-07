#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

void error(const char * message) {
  perror(message);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <server_hostname> <server_port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  
  //VARIABLES DECLARATIONS 
  int port, socket_fd, n;
  const char * hostname;
  char buffer[256];
  struct sockaddr_in server_addr;
  //VARIABLES PARSING
  hostname = argv[1];
  port = atoi(argv[2]);
  //CREATING THE socket
  socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  if (socket_fd < 0) {
    error("socket creation failed");
  }
  //check if the host exist
  // server = gethostbyname(hostname);
  // if(server == NULL) {
  //   error("no such host");
  // }
  //configure the server address struct
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  //convert the ipv4 address to the binary format
  if(inet_pton(AF_INET, hostname, &server_addr.sin_addr) <= 0) {
    error("invalid or unsupported address");
  }
  //connect to the server
  if(connect(socket_fd, 
             (struct sockaddr * )&server_addr, 
             sizeof(server_addr)) < 0) {
  error("ERROR connection failed");
  }
  //messagin part
  while (1) {
    //reading from the stdin
    printf("client : ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);

    if (strncmp(buffer, "exit", 4) == 0) {
      printf("exiting...\n");
      break;
    }
    n = write(socket_fd,buffer,strlen(buffer));
    if (n < 0) 
      error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(socket_fd,buffer,255);
    if (n < 0) {
      error("ERROR reading from socket");
    }
    else if (n == 0) {
      printf("Server has closed the connection. exiting...\n");
      break;
    }
    printf("server : %s",buffer);
  }
  printf("bye\n");
  close(socket_fd);
  return EXIT_SUCCESS;
}
