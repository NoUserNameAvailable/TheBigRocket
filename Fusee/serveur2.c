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

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

struct Array {
    int ** array;
    size_t used;
    size_t size;
};
typedef struct Array Array;

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

int tuerLesSegments(char * string, int shmid, int semid) {
    shmdt(string); // Detachement de la memoire partagee	
    shmctl(shmid, IPC_RMID, NULL); // Suppression de la memoire partagee
    semctl(semid, 0, IPC_RMID, NULL); // Suppression du groupe de semaphores
    exit(1);
}

void processusEnfant(char * query, int semid, int shmid_reserv) {
    printf("Message recu dans le fils : %s \n", query);

    fflush(stdout);
    char MessageRetour[2000];

    //Parse message
    int date = -1;
    int nb_ticket = 0;
    if (query[0] == 'C') {
        printf("Consultation \n");
        char * token;
        char * stringp = query;

        int j = 0;
        while (stringp != NULL) {
            token = strsep(&stringp, " ");
            if (j == 1) {
                date = atoi(token);
            } else if (j == 2) {
                nb_ticket = atoi(token);
            }
            j++;
        }
        printf("date %i, nbtickets %i", date, nb_ticket);

    } else if (query[0] == 'R') {
        printf("Réservation \n");
        char * token;
        char * stringp = query;

        int j = 0;
        while (stringp != NULL) {
            token = strsep(&stringp, " ");
            if (j == 1) {
                date = atoi(token);
            } else if (j == 2) {
                nb_ticket = atoi(token);
            }
            j++;
        }
        printf("date %i, nbtickets %i", date, nb_ticket);

    } else {
        printf("Mauvaise saisie client %s \n", query);
    }

    //Traitement de la Reservation
    if (date >= 0 && nb_ticket > 0 && query[0] == 'R' && date < 5000 && nb_ticket < 1000) {
        down(shmid_reserv, 0);

        Array * a;
        a = (Array*) shmat(shmid_reserv, NULL, SHM_W | SHM_R); // Attachement de la memoire partagee dans le pointeur memoire

        int indice = -1;
        int i = 0;
        for (i = 0; i < a->size; i++) {
            if (a->array[i][0] == date) {
                indice = i;
            }
        }
        //Realloc creation case pour une nouvelle date
        if (indice == -1) {
            a->array = (int**) realloc(a->array, (a->size + 1) * sizeof (int *));
            a->size = a->size + 1;
            a->array[a->size-1] = malloc(6 * sizeof (int));
            a->array[a->size-1][0] = date;
            a->array[a->size-1][1] = 8;
            a->array[a->size-1][2] = 14;
            a->array[a->size-1][3] = 14;
            a->array[a->size-1][4] = 8;
            a->array[a->size-1][5] = 8;
            indice = a->size-1;
        }


        if (indice != -1) {
            int nbPlaceDispo = 0;
            for (i = 1; i < 6; i++) {
                nbPlaceDispo = nbPlaceDispo + a->array[indice][i];
                printf("Nombre places disponibles pour le jour %d et horaire %d : %d \n", date, i, a->array[indice][i]);
                fflush(stdout);
            }
            printf("Nombre places disponibles pour le jour %d : %d \n", date, nbPlaceDispo);
            fflush(stdout);

            if (nbPlaceDispo > 0 && nbPlaceDispo >= nb_ticket) {
                int j = 1;
                for (j = 1; j < 6; j++) {
                    int placedispo = a->array[indice][j];
                    if (nb_ticket > 0 && placedispo > 0) {

                        if (placedispo < nb_ticket) {
                            nb_ticket = nb_ticket - placedispo;
                            printf("Places reservees pour l'horaire %d : %d (navette complete) \n", j, placedispo);
                            char ecrire[100];
                            sprintf(ecrire, "Places reservees pour l'horaire %d : %d (navette complete) \n", j, placedispo);
                            strcat(MessageRetour, ecrire);
                            fflush(stdout);
                            a->array[indice][j] = 0;
                        } else {
                            printf("Place reservees pour l'horaire %d : %d \n", j, nb_ticket);
                            char ecrire[100];
                            sprintf(ecrire, "Place reservees pour l'horaire %d : %d \n", j, nb_ticket);
                            strcat(MessageRetour, ecrire);
                            fflush(stdout);
                            nb_ticket = placedispo - nb_ticket;
                            a->array[indice][j] = nb_ticket;
                            nb_ticket = 0;
                        }
                    }
                }
            } else if (nbPlaceDispo < nb_ticket) { //Nombre des places demandées trop grand pour le jour souhaité
                printf("IMPOSSIBLE : pas assez de places(%d) pour le jour %d \n", nb_ticket, indice);
                char ecrire[100];
                sprintf(ecrire, "IMPOSSIBLE : pas assez de places(%d) pour le jour %d \n", nb_ticket, indice);
                strcat(MessageRetour, ecrire);
                fflush(stdout);
            }
        }

        sprintf(query,"%s", MessageRetour);

        fflush(stdout);
        up(shmid_reserv, 0);
        memset(MessageRetour, 0, 2000);

    }//Traitement de la consultation
    else if (date >= 0 && query[0] == 'C' && date < 5000 && nb_ticket < 1000) {
        down(shmid_reserv, 0);
        printf("\n");
        Array * a;
        a = (Array*) shmat(shmid_reserv, NULL, SHM_W | SHM_R); // Attachement de la memoire partagee dans le pointeur memoire

        int indice = -1;
        int i = 0;
        for (i = 0; i < a->size; i++) {
            if (a->array[i][0] == date) {
                indice = i;
            }
        }
        
        //Realloc creation case pour une nouvelle date
        if (indice == -1) {
            a->array = (int**) realloc(a->array, (a->size + 1) * sizeof (int *));
            a->size = a->size + 1;
            a->array[a->size-1] = malloc(6 * sizeof (int));
            a->array[a->size-1][0] = date;
            a->array[a->size-1][1] = 8;
            a->array[a->size-1][2] = 14;
            a->array[a->size-1][3] = 14;
            a->array[a->size-1][4] = 8;
            a->array[a->size-1][5] = 8;
            indice = a->size-1;
        }

        if (indice != -1) {
            int nbPlaceDispo = 0;
            for (i = 1; i < 6; i++) {
                nbPlaceDispo = nbPlaceDispo + a->array[indice][i];
                printf("Nombre places disponibles pour le jour %d et horaire %d : %d \n", date, i, a->array[indice][i]);
                fflush(stdout);
            }
            printf("Nombre places disponibles pour le jour %d : %d \n", date, nbPlaceDispo);
            char ecrire[100];
            sprintf(ecrire, "Nombre places disponibles pour le jour %d : %d \n", date, nbPlaceDispo);
            strcat(MessageRetour, ecrire);
            fflush(stdout);
        }
        sprintf(query,"%s", MessageRetour);

        fflush(stdout);
        up(shmid_reserv, 0);
        memset(MessageRetour, 0, 2000);

    }//Mauvais paramètres
    else {
        char ecrire[100];
        sprintf(ecrire, "Mauvaise saisie utilisateur");
        strcat(MessageRetour, ecrire);
        sprintf(query,"%s", MessageRetour);
        up(shmid_reserv, 0);
        memset(MessageRetour, 0, 2000);
    }

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
    initialize(shmid, 0, 0);

    //Tuer les processus mechants
    //killLePeuple(string, semid, shmid);

    //Initialisation pid
    pid_t pid;
    int status;


    //Strucutre pour gerer les reservations
    shmid_reserv = shmget(1999, sizeof (Array), IPC_CREAT | 0660);
    if (shmid_reserv == -1) {
        perror("Erreur lors du shmget_reserv");
        exit(-1);
    }

    Array * a = malloc(sizeof (Array));
    a = (Array *) shmat(shmid_reserv, NULL, SHM_W | SHM_R);

    a->array = malloc(5 * sizeof (int**));
    a->used = 0;
    a->size = 5;

    for (i = 0; i < 5; i++) {
        a->array[i] = malloc(6 * sizeof (int));
        a->array[i][0] = i;
        a->array[i][1] = 8;
        a->array[i][2] = 14;
        a->array[i][3] = 14;
        a->array[i][4] = 8;
        a->array[i][5] = 8;
    }
    up(shmid_reserv, 0);

    //initialize(shmid_reserv, 0,0);



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
                processusEnfant(string, semid, shmid_reserv);
                break;

            default:
                waitpid(pid, &status, 0);
                break;
        }

    }
    printf("serveur message recu %s \n", string);
    fflush(stdout);

    free(string);
    return 0;

}
