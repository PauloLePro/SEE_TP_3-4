//gcc -o ex6 ex6.c -lrt -pthread ./ex6 vec1 vec2

#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <aio.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <mqueue.h>

#define TAILLE_VECTEUR 400
#define NB_THREADS 3

#define QUEUE_NAME "/file_msg"
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 1024

mqd_t mq;
struct mq_attr attr_file;
char *buffer;
int mq_buffer = 0;
int compteur_thread = 0;

int indice_debut_vecteur[2];

typedef struct Str_gestion_vec
{
    int vecteur1[TAILLE_VECTEUR];
    int vecteur2[TAILLE_VECTEUR];

} Str_gestion_vec;

Str_gestion_vec struct_vec;

static void hdl(int sig, siginfo_t *siginfo, void *context)
{
    if (siginfo->si_value.sival_int == 0)
    {
        printf("handler read \n");
    }
    else if (siginfo->si_value.sival_int == 1)
    {
        printf("handler write \n");
    }
    else
    {
        perror("error hdl()");
    }
}

void init_vecteur(int *vecteur)
{
    int nbgen;

    for (size_t i = 0; i < TAILLE_VECTEUR; i++)
    {
        /*srand(time(NULL));
        int nbgen = rand() % 9 + 1;
        vecteur[i] = nbgen;*/

        vecteur[i] = 1;
    }
}

void *produit_scalaire(void *arg)
{
    int *data = (int *)arg;

    int resultat_intermediaires = 0;

    for (int i = 0; i < 200; i++)
    {
        resultat_intermediaires += struct_vec.vecteur1[indice_debut_vecteur[*data] + i] * struct_vec.vecteur2[indice_debut_vecteur[*data] + i];
    }

    if (mq_send(mq, (char *)&resultat_intermediaires, sizeof(resultat_intermediaires), 0) == -1)
    {
        perror("Error mq_send() :");
    }

    pthread_exit(NULL);
}

void *affiche_resultat()
{
    int result = 0;

    while (compteur_thread != (NB_THREADS - 1))
    {
        if (mq_receive(mq, buffer, mq_buffer, 0) == -1)
        {
            perror("mq_receive");
            pthread_exit(NULL);
        }
        compteur_thread++;
        result += (int)*buffer;
    }

    printf("resultat_global = %d\n", result);

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s {filenamevec1} {filenamevec2}\n", argv[0]);
        return -1;
    }

    int fd1, fd2;
    int valvec[TAILLE_VECTEUR];
    struct aiocb cb, cb1, cb2, cb3; // bloc de contrôle de l’E/S asynchrone
    const struct aiocb *cbs[4];
    struct sigaction act;
    memset(&act, '\0', sizeof(act));

    if ((fd1 = open(argv[1], O_CREAT | O_RDWR, 0644)) == -1)
    {
        perror("error open");
        return -1;
    }

    if ((fd2 = open(argv[2], O_CREAT | O_RDWR, 0644)) == -1)
    {
        perror("error open");
        return -1;
    }

    init_vecteur(valvec);

    if (write(fd1, valvec, sizeof(int) * TAILLE_VECTEUR) != (sizeof(int) * TAILLE_VECTEUR))
    {
        perror("error write");
        return -1;
    }

    if (write(fd2, valvec, sizeof(int) * TAILLE_VECTEUR) != (sizeof(int) * TAILLE_VECTEUR))
    {
        perror("error write");
        return -1;
    }

    act.sa_sigaction = &hdl;
    act.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGRTMIN, &act, NULL) < 0)
    {
        perror("error sigaction");
        return -1;
    }

    int taille = ((sizeof(int)) * TAILLE_VECTEUR) / 2;

    if ((cb.aio_buf = malloc(taille)) == NULL)
    {
        perror("malloc");
    }
    cb.aio_fildes = fd1;
    cb.aio_nbytes = taille;
    cb.aio_offset = 0;
    cb.aio_reqprio = 0;
    cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    cb.aio_sigevent.sigev_signo = SIGRTMIN;
    cb.aio_sigevent.sigev_value.sival_int = 0;

    if ((cb1.aio_buf = malloc(taille)) == NULL)
    {
        perror("malloc");
    }
    cb1.aio_fildes = fd1;
    cb1.aio_nbytes = taille;
    cb1.aio_offset = taille;
    cb1.aio_reqprio = 0;
    cb1.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    cb1.aio_sigevent.sigev_signo = SIGRTMIN;
    cb1.aio_sigevent.sigev_value.sival_int = 0;

    if ((cb2.aio_buf = malloc(taille)) == NULL)
    {
        perror("malloc");
    }
    cb2.aio_fildes = fd2;
    cb2.aio_nbytes = taille;
    cb2.aio_offset = 0;
    cb2.aio_reqprio = 0;
    cb2.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    cb2.aio_sigevent.sigev_signo = SIGRTMIN;
    cb2.aio_sigevent.sigev_value.sival_int = 0;

    if ((cb3.aio_buf = malloc(taille)) == NULL)
    {
        perror("malloc");
    }
    cb3.aio_fildes = fd2;
    cb3.aio_nbytes = taille;
    cb3.aio_offset = taille;
    cb3.aio_reqprio = 0;
    cb3.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    cb3.aio_sigevent.sigev_signo = SIGRTMIN;
    cb3.aio_sigevent.sigev_value.sival_int = 0;

    if (aio_read(&cb) == -1)
    {
        perror("error aio_read cb:");
    }
    if (aio_read(&cb1) == -1)
    {
        perror("error aio_read cb1:");
    }
    if (aio_read(&cb2) == -1)
    {
        perror("error aio_read cb2:");
    }
    if (aio_read(&cb3) == -1)
    {
        perror("error aio_read cb3:");
    }

    //Suspension du processus dans l’attente de la terminaison de la lecture
    cbs[0] = &cb;
    cbs[1] = &cb1;
    cbs[2] = &cb2;
    cbs[3] = &cb3;

    if (aio_suspend(cbs, 4, 0) == -1)
    {
        perror("error aio_suspend :");
    }

    if (aio_return(&cb) == -1)
    {
        perror("error aio_return cb:");
    }
    else
    {
        printf("(lecture) operation AIO a retouree %d\n", aio_return(&cb));
    }

    if (aio_return(&cb1) == -1)
    {
        perror("error aio_return cb1:");
    }
    else
    {
        printf("(lecture 1) operation AIO a retouree %d\n", aio_return(&cb1));
    }

    if (aio_return(&cb2) == -1)
    {
        perror("error aio_return cb2:");
    }
    else
    {
        printf("(lecture 2) operation AIO a retouree %d\n", aio_return(&cb2));
    }

    if (aio_return(&cb3) == -1)
    {
        perror("error aio_return cb3:");
    }
    else
    {
        printf("(lecture 3) operation AIO a retouree %d\n", aio_return(&cb3));
    }

    memcpy((char *)struct_vec.vecteur1, (char *)cb.aio_buf, taille);
    memcpy(&((char *)struct_vec.vecteur1)[taille], (char *)cb1.aio_buf, taille);
    memcpy((char *)struct_vec.vecteur2, (char *)cb2.aio_buf, taille);
    memcpy(&((char *)struct_vec.vecteur2)[taille], (char *)cb3.aio_buf, taille);

    pthread_t thread[NB_THREADS];
    pthread_attr_t attr, attr_aff;
    int rc, rc_aff, t, fd;
    void *status;

    indice_debut_vecteur[0] = 0;
    indice_debut_vecteur[1] = 200;

    attr_file.mq_flags = 0;
    attr_file.mq_maxmsg = MAX_MESSAGES;
    attr_file.mq_msgsize = MAX_MSG_SIZE;
    attr_file.mq_curmsgs = 0;

    if ((mq = mq_open(QUEUE_NAME, O_RDWR | O_CREAT, 0664, &attr_file)) == -1)
    {
        perror("Error mq_open() :");
        exit(1);
    }

    if (mq_getattr(mq, &attr_file) != 0)
    {
        perror("error mq_getattr");
        return -1;
    }

    mq_buffer = attr_file.mq_msgsize;

    buffer = malloc(mq_buffer);
    if (buffer == NULL)
    {
        perror("error malloc");
        return -1;
    }

    pthread_attr_init(&attr);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    pthread_attr_init(&attr_aff);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr_aff, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    int id_thread[2] = {0, 1};

    for (int t = 0; t < (NB_THREADS - 1); t++)
    {
        printf("Creation du thread %d\n", t);
        rc = pthread_create(&thread[t], &attr, produit_scalaire, &id_thread[t]);
        if (rc)
        {
            printf("ERROR; le code de retour de pthread_create() est %d\n", rc);
            exit(-1);
        }
    }

    printf("Creation du thread %d\n", NB_THREADS - 1);
    rc_aff = pthread_create(&thread[NB_THREADS - 1], &attr_aff, affiche_resultat, NULL);
    if (rc_aff)
    {
        printf("ERROR; le code de retour de pthread_create() est %d\n", rc);
        exit(-1);
    }

    pthread_attr_destroy(&attr);
    for (t = 0; t < NB_THREADS; t++)
    {
        rc = pthread_join(thread[t], &status);
        if (rc)
        {
            printf("ERROR; le code de retour du pthread_join() est %d\n", rc);
            exit(-1);
        }
        printf("le join a fini avec le thread %d et a donne le status= %ld\n", t, (long)status);
    }

    pthread_exit(NULL);

    mq_unlink(QUEUE_NAME);
    mq_close(mq);

    close(fd1);
    close(fd2);

    return 0;
}