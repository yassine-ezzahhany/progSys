CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
LDFLAGS = -pthread

# Noms des exécutables cibles
TARGETS = client server client_nb server_nb

# Règle par défaut
all: $(TARGETS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client_nb: client_nb.c
	$(CC) $(CFLAGS) -o client_nb client_nb.c $(LDFLAGS)

server_nb: server_nb.c
	$(CC) $(CFLAGS) -o server_nb server_nb.c $(LDFLAGS)

# Nettoyage des exécutables générés
clean:
	rm -f $(TARGETS)

.PHONY: all clean
