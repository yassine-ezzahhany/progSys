#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sched.h>
#include <termios.h>

void error(const char *message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

typedef struct {
  int fd;
  pthread_t reader_tid;
  pthread_t writer_tid;
  pthread_mutex_t lock;
  char input_buffer[256];
  int input_len;
} shats_args_t;

// Terminal mode variables
struct termios orig_termios;

void enable_raw_mode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  struct termios raw = orig_termios;
  // Disable Canonical mode (line buffering) and ECHO (auto-printing keys)
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void disable_raw_mode() {
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void *shats_reader(void *arg);
void *shats_writer(void *arg);
void shats(int fd);

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int port, socket_fd, new_socket_fd;
  const char *hostname;
  socklen_t clilen;
  struct sockaddr_in server_addr, client_addr;

  hostname = argv[1];
  port = atoi(argv[2]);

  socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (socket_fd < 0)
  {
    error("ERROR on socket creation");
  }

  // Prevent "Address already in use" errors on restart
  int opt = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  
  if(inet_pton(AF_INET, hostname, &server_addr.sin_addr) < 0) {
    error("ERROR invalid or unsupported format");
  }
  
  server_addr.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    error("ERROR on binding");

  printf("Listening for incoming connections on %s:%d...\n", hostname, port);
  listen(socket_fd, 1);

  while (1) {
    clilen = sizeof(client_addr);
    new_socket_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &clilen);
    if (new_socket_fd < 0) {
      error("ERROR on accept");
    }

    printf("Client has connected from IP %s and PORT %d\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));

    shats(new_socket_fd);
    close(new_socket_fd);
    printf("Waiting for next client...\n");
  }

  close(socket_fd);
  return EXIT_SUCCESS;
}

void *shats_reader(void *arg)
{
  shats_args_t *args = arg;
  char buffer[256];
  ssize_t n;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  while (1)
  {
    bzero(buffer, sizeof(buffer));
    n = read(args->fd, buffer, sizeof(buffer) - 1);
    
    if (n < 0) {
      pthread_mutex_lock(&args->lock);
      printf("\r\033[KERROR reading from socket\n");
      fflush(stdout);
      pthread_mutex_unlock(&args->lock);
      break;
    }
    else if (n == 0) {
      pthread_mutex_lock(&args->lock);
      printf("\r\033[K\nClient has exited. Shutting down...\n");
      fflush(stdout);
      pthread_mutex_unlock(&args->lock);
      break;
    }
    
    buffer[n] = '\0';
    
    pthread_mutex_lock(&args->lock);
    
    // Clear the current line (where user might be typing)
    printf("\r\033[K");
    
    // Print the incoming message
    printf("Client : %s\n", buffer);
    
    // Redraw the prompt and exactly what the user had typed so far
    printf("Server : %s", args->input_buffer);
    fflush(stdout);
    
    pthread_mutex_unlock(&args->lock);
  }

  while (args->writer_tid == 0)
    sched_yield();
  pthread_cancel(args->writer_tid);
  return NULL;
}

void *shats_writer(void *arg)
{
  shats_args_t *args = arg;
  char c;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  pthread_mutex_lock(&args->lock);
  printf("Server : ");
  fflush(stdout);
  pthread_mutex_unlock(&args->lock);

  while (1)
  {
    // Block until exactly ONE key is pressed
    if (read(STDIN_FILENO, &c, 1) == 1) {
      pthread_mutex_lock(&args->lock);
      
      if (c == '\n') {
        // The user pressed Enter
        printf("\n");
        args->input_buffer[args->input_len] = '\0';
        
        if (strncmp(args->input_buffer, "exit", 4) == 0) {
          printf("Exiting...\n");
          fflush(stdout);
          pthread_mutex_unlock(&args->lock);
          break;
        }

        // Send the message over the socket
        write(args->fd, args->input_buffer, args->input_len);
        
        // Reset the buffer for the next message
        bzero(args->input_buffer, sizeof(args->input_buffer));
        args->input_len = 0;
        
        // Print the prompt again
        printf("Server : ");
        fflush(stdout);
        
      } 
      else if (c == 127 || c == '\b') { 
        // The user pressed Backspace
        if (args->input_len > 0) {
          args->input_len--;
          args->input_buffer[args->input_len] = '\0';
          // Move cursor back, print a space to erase, move cursor back again
          printf("\b \b"); 
          fflush(stdout);
        }
      } 
      else {
        // Normal typing: save it to the struct and echo it to the screen
        if (args->input_len < sizeof(args->input_buffer) - 1) {
          args->input_buffer[args->input_len++] = c;
          args->input_buffer[args->input_len] = '\0';
          putchar(c); 
          fflush(stdout);
        }
      }
      pthread_mutex_unlock(&args->lock);
    }
  }

  while (args->reader_tid == 0)
    sched_yield();
  pthread_cancel(args->reader_tid);
  return NULL;
}

void shats(int fd)
{
  shats_args_t args;
  pthread_t reader_tid = 0;
  pthread_t writer_tid = 0;

  args.fd = fd;
  args.reader_tid = 0;
  args.writer_tid = 0;
  args.input_len = 0;
  pthread_mutex_init(&args.lock, NULL);
  bzero(args.input_buffer, sizeof(args.input_buffer));

  // Bypass OS line-buffering to achieve the asynchronous UI
  enable_raw_mode(); 

  if (pthread_create(&writer_tid, NULL, shats_writer, &args) != 0)
    error("ERROR creating writer thread");
  args.writer_tid = writer_tid;

  if (pthread_create(&reader_tid, NULL, shats_reader, &args) != 0)
    error("ERROR creating reader thread");
  args.reader_tid = reader_tid;

  // Wait for both threads to cleanly exit/cancel
  pthread_join(writer_tid, NULL);
  pthread_join(reader_tid, NULL);

  // Restore the terminal to normal behavior so your bash prompt works again
  disable_raw_mode(); 

  pthread_mutex_destroy(&args.lock);
  printf("Bye!\n");
}