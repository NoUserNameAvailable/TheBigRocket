#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

int down(int sem_id, int sem_num) {// Mise en écoute 
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;

    if (semop(sem_id, &sem_op, 1) == -1) {
        printf("Erreur lors du down \n");
        return -1;
    } else return 0;
}

int up(int sem_id, int sem_num) {// Message envoyé
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = +1;
    sem_op.sem_flg = 0;

    if (semop(sem_id, &sem_op, 1) == -1) {
        printf("Erreur lors du up\n");
        return -1;
    }
}

int main() {
    int i = 0, semid, shmid;
    char *string;
    string = malloc(4096 * sizeof(char));
    char *string_in;
    string_in = malloc(4096 *sizeof(char));


    pid_t pid;

    // Initialisation sémaphore
    semid = semget(1234, 1, 0660);
    if (semid == -1) {
        perror("Erreur lors du semget");
        exit(-1);
    }

    // Initialisation shared memory
    shmid = shmget(13245, 4096 * sizeof (char), 0660); // Creation de la memoire partage
    if (shmid == -1) {
        perror("Erreur lors du shmget");
        exit(-1);
    }

    string = (char*) shmat(shmid, NULL, SHM_W | SHM_R); // Attachement de la memoire partagee dans le pointeur memoire

    while (1) {// Boucle infinie pour échanger avec le serveur

        fgets (string_in, 4096, stdin); // Récupère la string saisie
        if ((strlen(string_in)>0) && (string_in[strlen (string_in) - 1] == '\n'))
        string_in[strlen (string_in) - 1] = '\0';
        
        sprintf(string, "%s", string_in);
        printf("String envoyee %s\n", string_in);
        up(semid, 0);
        down(semid, 0);
        printf("Message du serveur : \n%s\n", string);   
    }

    up(semid, 0);

    return 0;

}
