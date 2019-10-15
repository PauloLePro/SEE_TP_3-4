//gcc ex4 ex4.c -o -lpthread -lrt

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>    /* Pour les constantes O_* */
#include <sys/stat.h> /* Pour les constantes « mode » */
#include <mqueue.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define NB_THREADS 5
#define TAILLE_VECTEUR 400

#define QUEUE_NAME "/file_msg"
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */
pthread_cond_t tlm_rep;

pthread_mutex_t mutex_aff = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */

pthread_mutex_t mutex_thread_id = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */

int indice_debut_vecteur[4];

mqd_t mq;
struct mq_attr attr_file;
char buffer[MAX_MSG_SIZE + 1];

typedef struct Str_gestion_vec
{
    int vecteur1[TAILLE_VECTEUR];
    int vecteur2[TAILLE_VECTEUR];

    int resultat_intermediaires;
    int compteur_thread;
    int thread_id;
} Str_gestion_vec;

Str_gestion_vec struct_vec;

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

void *produit_scalaire()
{
    for (int i = 0; i < 100; i++)
    {
        struct_vec.resultat_intermediaires += struct_vec.vecteur1[indice_debut_vecteur[struct_vec.thread_id] + i] * struct_vec.vecteur2[indice_debut_vecteur[struct_vec.thread_id] + i];
    }

    pthread_mutex_lock(&mutex); // On verrouille le mutex
    printf("resultat intermédiaire = %d\n", struct_vec.resultat_intermediaires);

    if (mq_send(mq, (char *)&struct_vec.resultat_intermediaires, sizeof(struct_vec.resultat_intermediaires), 0) == -1)
    {
        perror("Error mq_send() :");
    }
    struct_vec.resultat_intermediaires = 0;
    struct_vec.compteur_thread++;

    if (struct_vec.compteur_thread == (NB_THREADS - 1))
    {
        pthread_cond_signal(&tlm_rep);
    }
    pthread_mutex_unlock(&mutex); // On déverrouille le mutex

    pthread_exit((void *)0);
}

void *affiche_resultat()
{
    int result = 0;

    pthread_mutex_lock(&mutex_aff);
    while (struct_vec.compteur_thread != (NB_THREADS - 1))
    {
        pthread_cond_wait(&tlm_rep, &mutex_aff);
    }

    for (size_t i = 0; i < NB_THREADS - 1; i++)
    {
        if (mq_receive(mq, buffer, MAX_MSG_SIZE + 1, 0) == -1)
        {
            perror("Error mq_receive() :");
            exit(1);
        }

        //result += ((int *)buffer)[i];
        result += (int)*buffer;
    }

    printf("resultat_global = %d\n", result);
    pthread_mutex_unlock(&mutex_aff);
}

int main(int argc, char *argv[])
{
    pthread_t thread[NB_THREADS];
    pthread_attr_t attr, attr_aff;
    int rc, rc_aff, t, fd;
    void *status;

    /* Initialisation de la struct*/
    init_vecteur(struct_vec.vecteur1);
    init_vecteur(struct_vec.vecteur2);

    indice_debut_vecteur[0] = 0;
    indice_debut_vecteur[1] = 100;
    indice_debut_vecteur[2] = 200;
    indice_debut_vecteur[3] = 300;

    struct_vec.resultat_intermediaires = 0;
    struct_vec.compteur_thread = 0;
    struct_vec.thread_id = 0;

    attr_file.mq_flags = 0;
    attr_file.mq_maxmsg = MAX_MESSAGES;
    attr_file.mq_msgsize = MAX_MSG_SIZE;
    attr_file.mq_curmsgs = 0;

    if ((mq = mq_open(QUEUE_NAME, O_RDWR | O_CREAT, 0664, &attr_file)) == -1)
    {
        perror("Error mq_open() :");
        exit(1);
    }

    /* Initialisation et activation d’attributs lié au threads*/
    pthread_attr_init(&attr);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    pthread_attr_init(&attr_aff);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr_aff, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutex_aff, NULL);
    pthread_mutex_init(&mutex_thread_id, NULL);

    pthread_cond_init(&tlm_rep, NULL);

    for (int t = 0; t < (NB_THREADS - 1); t++)
    {
        pthread_mutex_lock(&mutex_thread_id);
        printf("Creation du thread %d\n", t);
        indice_debut_vecteur[struct_vec.thread_id];
        rc = pthread_create(&thread[t], &attr, produit_scalaire, NULL);
        struct_vec.thread_id++;
        if (rc)
        {
            printf("ERROR; le code de retour de pthread_create() est %d\n", rc);
            exit(-1);
        }
        pthread_mutex_unlock(&mutex_thread_id);
    }

    printf("Creation du thread %d\n", NB_THREADS - 1);
    rc_aff = pthread_create(&thread[(NB_THREADS - 1)], &attr_aff, affiche_resultat, NULL);
    if (rc_aff)
    {
        printf("ERROR; le code de retour de pthread_create() est %d\n", rc);
        exit(-1);
    }

    /* liberation des attributs et attente de la terminaison des threads */
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

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&tlm_rep);
    pthread_exit(NULL);

    mq_unlink(QUEUE_NAME);
    mq_close(mq);

    return 0;
}