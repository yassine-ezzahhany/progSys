CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE

# Noms des exécutables cibles
TARGETS = exercice1 exercice2 exercice3 exercice4 exercice5 exercice6

# Règle par défaut
all: $(TARGETS)

exercice1: exercice1.c
	$(CC) $(CFLAGS) -o exercice1 exercice1.c

exercice2: exercice2.c
	$(CC) $(CFLAGS) -o exercice2 exercice2.c

exercice3: exercice3.c
	$(CC) $(CFLAGS) -o exercice3 exercice3.c

exercice4: exercice4.c
	$(CC) $(CFLAGS) -o exercice4 exercice4.c

# Pour l'exercice 5, on a besoin de l'environnement POSIX standard
exercice5: exercice5.c
	$(CC) $(CFLAGS) -o exercice5 exercice5.c

# Pour l'exercice 6, on a besoin des bibliothèques système Unix
exercice6: exercice6.c
	$(CC) $(CFLAGS) -o exercice6 exercice6.c

# Nettoyage des exécutables et des fichiers temporaires générés
clean:
	rm -f $(TARGETS) testFile.txt

.PHONY: all clean
