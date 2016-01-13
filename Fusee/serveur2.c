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

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

int initialize(int sem_id, int sem_num, int init) {
    union semun semunion;

    semunion.val = init;
    if (semctl(sem_id, sem_num, SETVAL, semunion) == -1) return -1;
    else return 0;
}

int down(int sem_id, int sem_num) {
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;

    if (semop(sem_id, &sem_op, 1) == -1) return -1;
    else return 0;
}

int up(int sem_id, int sem_num) {
    struct sembuf sem_op;
    sem_op.sem_num = sem_num;
    sem_op.sem_op = +1;
    sem_op.sem_flg = 0;

    if (semop(sem_id, &sem_op, 1) == -1) return -1;
}

int killLePeuple(char * string, int shmid, int semid) {
    shmdt(string); // Detachement de la memoire partagee	
    shmctl(shmid, IPC_RMID, NULL); // Suppression de la memoire partagee
    semctl(semid, 0, IPC_RMID, NULL); // Suppression du groupe de semaphores
    exit(1);
}

void processusEnfant(char * query, int semid) {
    printf("Message recu dans le fils : %s \n", query);
    sprintf(query, "%s", "holle \n");
    fflush(stdout);
    up(semid, 0);
}

int main() {
    int i = 0, semid, shmid, shmid_reserv;
    char *string;
    string = malloc(4096 * sizeof (char));

    // creation semaphore
    semid = semget(1234, 1, IPC_CREAT | 0660); // Creation d'un groupe contenant 2 semaphore	
    if (semid == -1) {
        perror("Erreur lors du semget");
        exit(-1);
    }

    initialize(semid, 0, 0);

    // creation memoire partagee
    shmid = shmget(13245, 4096 * sizeof (char), IPC_CREAT | 0660); // Creation de la memoire partagee
    if (shmid == -1) {
        perror("Erreur lors du shmget");
        exit(-1);
    }

    //Tuer les processus mechants
    //killLePeuple(string, semid, shmid);

    //Initialisation pid
    pid_t pid;
    int status;

    //Creation du tableau pour les reservations
    //    int ** reserv = (int **) shmat(shmid_reserv, NULL, 0);
    //    const size_t row_pointers_bytes = 5 * sizeof(*reserv);
    //    const size_t row_elements_bytes = 5 * sizeof(**reserv);
    //    reserv = malloc(row_pointers_bytes + 5 * row_elements_bytes);
    //    reserv[0][0] = 3;
    //    printf("Tableau début %i", reserv[0][0]);
    //    fflush(stdout);

    
    //Test
    int ** array;
    array = (int **) shmat(shmid, 0, 0);

    array = malloc(5 * sizeof (int *));
    for(int i=0; i<5; i++){
        array[i] = malloc(5 *sizeof(int));
    }
    array[0][0] = 3;
    
    printf("%i", array[0][0]);
    fflush(stdout);
    //fin test

    while (1) {
        string = (char*) shmat(shmid, NULL, SHM_W | SHM_R); // Attachement de la memoire partagee dans le pointeur memoire

        down(semid, 0);

        pid = fork();
        switch (pid) {
            case -1:
                perror("fork");
                return EXIT_FAILURE;
                break;

            case 0:
                processusEnfant(string, semid);
                break;

            default:
                waitpid(pid, &status, 0);
                break;
        }

    }
    printf("serveur message recu %s \n", string);
    fflush(stdout);

    //sprintf(string, "%s", string);

    //    printf("%s", string);
    //    fflush(stdout);
    //    printf("\n");
    //    fflush(stdout);
    //    up(semid, 0);
    //        down(semid, 0);
    //        up(semid, 0);



    return 0;

}
