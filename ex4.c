//gcc -o ex4 ex4.c -lpthread -lrt

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

int indice_debut_vecteur[4];

mqd_t mq;
struct mq_attr attr_file;
char *buffer;
int mq_buffer = 0;
int compteur_thread = 0;

typedef struct Str_gestion_vec
{
    int vecteur1[TAILLE_VECTEUR];
    int vecteur2[TAILLE_VECTEUR];

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

void *produit_scalaire(void *arg)
{
    int *data = (int *)arg;

    int resultat_intermediaires = 0;

    for (int i = 0; i < 100; i++)
    {
        resultat_intermediaires += struct_vec.vecteur1[indice_debut_vecteur[*data] + i] * struct_vec.vecteur2[indice_debut_vecteur[*data] + i];
    }

    printf("%d resultat intermédiaire = %d\n", *data, resultat_intermediaires);

    if (mq_send(mq, (char *)&resultat_intermediaires, sizeof(resultat_intermediaires), 0) == -1)
    {
        perror("Error mq_send() :");
    }

    resultat_intermediaires = 0;

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

    /* Initialisation et activation d’attributs lié au threads*/
    pthread_attr_init(&attr);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    pthread_attr_init(&attr_aff);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr_aff, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    int id_thread[4] = {0, 1, 2, 3};

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

    pthread_exit(NULL);

    mq_unlink(QUEUE_NAME);
    mq_close(mq);

    return 0;
}