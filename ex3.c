//gcc ex3 ex3.c -o -lpthread -lrt

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <error.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#define NB_THREADS 5
#define TAILLE_VECTEUR 400
#define NOM_MEM "/Mem_vecteur"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */
pthread_cond_t tlm_rep;

pthread_mutex_t mutex_aff = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */

pthread_mutex_t mutex_thread_id = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */

int indice_debut_vecteur[4];

typedef struct Str_gestion_vec
{
    int vecteur1[TAILLE_VECTEUR];
    int vecteur2[TAILLE_VECTEUR];

    int resultat_final;
    int resultat_intermediaires;
    int compteur_thread;
    int thread_id;
} Str_gestion_vec;

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

void *produit_scalaire(void *param)
{
    Str_gestion_vec *data = (Str_gestion_vec *)param;

    for (int i = 0; i < 100; i++)
    {
        data->resultat_intermediaires += data->vecteur1[indice_debut_vecteur[data->thread_id] + i] * data->vecteur2[indice_debut_vecteur[data->thread_id] + i];
    }

    pthread_mutex_lock(&mutex); // On verrouille le mutex
    printf("resultat intermédiaire = %d\n", data->resultat_intermediaires);

    data->resultat_final += data->resultat_intermediaires; //on ajoute le résultat intermediaires dans le final
    data->resultat_intermediaires = 0;
    data->compteur_thread++;

    if (data->compteur_thread == (NB_THREADS - 1))
    {
        pthread_cond_signal(&tlm_rep);
    }
    pthread_mutex_unlock(&mutex); // On déverrouille le mutex

    pthread_exit((void *)0);
}

void *affiche_resultat(void *param)
{

    Str_gestion_vec *data = (struct Str_gestion_vec *)param;

    pthread_mutex_lock(&mutex_aff);
    while (data->compteur_thread != (NB_THREADS - 1))
    {
        pthread_cond_wait(&tlm_rep, &mutex_aff);
    }
    printf("resultat_global = %d\n", data->resultat_final);
    pthread_mutex_unlock(&mutex_aff);
}

int main(int argc, char *argv[])
{
    pthread_t thread[NB_THREADS];
    pthread_attr_t attr, attr_aff;
    int rc, rc_aff, t, fd;
    void *status;

    Str_gestion_vec struct_vec;

    /* Initialisation de la struct*/
    init_vecteur(struct_vec.vecteur1);
    init_vecteur(struct_vec.vecteur2);

    indice_debut_vecteur[0] = 0;
    indice_debut_vecteur[1] = 100;
    indice_debut_vecteur[2] = 200;
    indice_debut_vecteur[3] = 300;

    struct_vec.resultat_final = 0;
    struct_vec.resultat_intermediaires = 0;
    struct_vec.compteur_thread = 0;
    struct_vec.thread_id = 0;

    /* shm_open créer objets de mémoire partagés POSIX */
    fd = shm_open(NOM_MEM, O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        perror("Error shm_open() :");
        return -1;
    }

    /* ftruncate - Tronquer un fichier à une longueur donnée */
    if (ftruncate(fd, sizeof(Str_gestion_vec)) != 0)
    {
        perror("Error ftruncate() :");
        return -1;
    }

    /* Établie une projection en mémoire (map) des fichiers ou des périphériques  */
    void *mmappedData = mmap(NULL, sizeof(Str_gestion_vec), PROT_WRITE, MAP_SHARED, fd, 0);

    if (mmappedData == MAP_FAILED)
    {
        perror("Error mmap :");
        return -1;
    }

    /*écriture de la structure dans la memoire partager*/
    memcpy(mmappedData, &struct_vec, sizeof(Str_gestion_vec));

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
        rc = pthread_create(&thread[t], &attr, produit_scalaire, mmappedData);
        struct_vec.thread_id++;
        if (rc)
        {
            printf("ERROR; le code de retour de pthread_create() est %d\n", rc);
            exit(-1);
        }
        pthread_mutex_unlock(&mutex_thread_id);
    }

    printf("Creation du thread %d\n", NB_THREADS - 1);
    rc_aff = pthread_create(&thread[(NB_THREADS - 1)], &attr_aff, affiche_resultat, mmappedData);
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

    close(fd);
    shm_unlink(NOM_MEM);
    munmap(mmappedData, sizeof(Str_gestion_vec));

    return 0;
}