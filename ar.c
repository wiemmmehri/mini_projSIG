#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>

#define NUM_CHILDREN 4

pid_t children[NUM_CHILDREN];
sem_t *sem_start;
int confirmations = 0;

void handle_signal(int sig) {
    if (sig == SIGUSR1) {
        confirmations++;
        printf("Parent received confirmation signal (SIGUSR1). Total confirmations: %d\n", confirmations);
    }
}

void child_task(int child_num) {
    // Attendre le signal de départ
    sem_wait(sem_start);
    printf("Child %d starting task.\n", child_num);

    // Simuler une tâche complexe avec une boucle et sleep
    for (int i = 0; i < 5; i++) {
        printf("Child %d working...\n", child_num);
        sleep(1);
    }

    // Envoyer un signal de confirmation au père
    kill(getppid(), SIGUSR1);
    exit(0);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    sem_start = sem_open("/sem_start", O_CREAT | O_EXCL, 0644, 0);
    if (sem_start == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            child_task(i + 1);
        } else if (pid > 0) {
            children[i] = pid;
        } else {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    // Envoyer un signal de départ à tous les processus fils
    for (int i = 0; i < NUM_CHILDREN; i++) {
        sem_post(sem_start);
    }

    // Attendre les confirmations des processus fils
    while (confirmations < NUM_CHILDREN) {
        pause(); // Attendre un signal
    }

    // Nettoyer et fermer les sémaphores
    sem_close(sem_start);
    sem_unlink("/sem_start");

    // Attendre que tous les fils terminent
    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    printf("Parent process exiting.\n");
    return 0;
}
