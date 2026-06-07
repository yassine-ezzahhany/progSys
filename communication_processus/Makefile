CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE

# Noms des exécutables cibles
TARGETS = exercice1 exercice2 exercice3

# Règle par défaut
all: $(TARGETS)

exercice1: exercice1.c
	$(CC) $(CFLAGS) -o exercice1 exercice1.c

exercice2: exercice2.c
	$(CC) $(CFLAGS) -o exercice2 exercice2.c

exercice3: exercice3.c
	$(CC) $(CFLAGS) -o exercice3 exercice3.c

# Nettoyage des exécutables et des fichiers temporaires générés
clean:
	rm -f $(TARGETS) sauvegarde.txt

.PHONY: all clean
