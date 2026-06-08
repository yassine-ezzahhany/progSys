CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c99 -D_GNU_SOURCE
DEPS = common.h
OBJ_SERVER = server.o common.o
OBJ_CLIENT = client.o common.o

all: chat_server chat_client

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

chat_server: $(OBJ_SERVER)
	$(CC) -o $@ $^ $(CFLAGS)

chat_client: $(OBJ_CLIENT)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o chat_server chat_client chat_history.log
