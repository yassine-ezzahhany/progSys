#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct Application {
    char name[100];
    pid_t pid;
    struct Application* next;
} Application;

Application* creer_noeud(const char* name, pid_t pid);
Application* ajouter_application(Application* head, const char* name, pid_t pid);
void afficher_liste(Application* head);
void detruire_liste(Application* head);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Utilisation: %s <n_copies> <prog1> [prog2 ...]\n", argv[0]);
        return 1;
    }

    int n_copies = atoi(argv[1]);
    if (n_copies <= 0) {
        fprintf(stderr, "Erreur: n_copies doit etre > 0\n");
        return 1;
    }

    Application* liste_apps = NULL;

    for (int i = 2; i < argc; i++) {
        char* nom_prog = argv[i];
        
        for (int j = 0; j < n_copies; j++) {
            pid_t pid_reel = fork();
            
            if (pid_reel < 0) {
                perror("fork");
            } 
            else if (pid_reel == 0) {
                freopen("/dev/null", "w", stdout); 
                freopen("/dev/null", "w", stderr); 
                execlp(nom_prog, nom_prog, NULL);
                exit(1); 
            } 
            else {
                liste_apps = ajouter_application(liste_apps, nom_prog, pid_reel);
            }
        }
    }

    int status;
    while (wait(&status) > 0);

    afficher_liste(liste_apps);

    detruire_liste(liste_apps);
    liste_apps = NULL; 

    return 0;
}

Application* creer_noeud(const char* name, pid_t pid) {
    Application* nouveau = (Application*)malloc(sizeof(Application));
    if (nouveau == NULL) {
        exit(EXIT_FAILURE);
    }
    
    strncpy(nouveau->name, name, sizeof(nouveau->name) - 1);
    nouveau->name[sizeof(nouveau->name) - 1] = '\0'; 
    
    nouveau->pid = pid;
    nouveau->next = NULL; 
    
    return nouveau;
}

Application* ajouter_application(Application* head, const char* name, pid_t pid) {
    Application* nouveau = creer_noeud(name, pid);

    if (head == NULL) {
        return nouveau;
    }

    Application* courant = head;
    while (courant->next != NULL) {
        courant = courant->next;
    }
    
    courant->next = nouveau;
    return head;
}

void afficher_liste(Application* head) {
    Application* courant = head;
    
    if (courant == NULL) {
        printf("NULL\n");
        return;
    }

    while (courant != NULL) {
        printf("[%s | %d] -> ", courant->name, courant->pid);
        courant = courant->next;
    }
    printf("NULL\n"); 
}

void detruire_liste(Application* head) {
    Application* courant = head;
    Application* suivant;

    while (courant != NULL) {
        suivant = courant->next; 
        kill(courant->pid, SIGKILL); 
        free(courant);           
        courant = suivant;       
    }
}