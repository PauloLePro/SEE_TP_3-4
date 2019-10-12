//gcc -o ex1 ex1.c -lpthread

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define NB_THREADS 5
#define TAILLE_VECTEUR 400

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */
pthread_cond_t tlm_rep;

pthread_mutex_t mutex_aff = PTHREAD_MUTEX_INITIALIZER; /* Création du mutex */

int vecteur1[TAILLE_VECTEUR];
int vecteur2[TAILLE_VECTEUR];

int resultat_global = 0;
int compteur_thread = 0;

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
    int indice_debut = *(int *)param;
    int resultat = 0;

    for (int i = 0; i < 100; i++)
    {
        resultat += vecteur1[indice_debut + i] * vecteur2[indice_debut + i];
    }

    pthread_mutex_lock(&mutex); /* On verrouille le mutex */
    resultat_global += resultat;
    printf("resultat intermédiaire = %d\n", resultat);
    compteur_thread++;
    if (compteur_thread == (NB_THREADS - 1))
    {
        pthread_cond_signal(&tlm_rep);
    }
    pthread_mutex_unlock(&mutex); /* On déverrouille le mutex */

    pthread_exit((void *)0);
}

void *affiche_resultat()
{
    pthread_mutex_lock(&mutex_aff);
    while (compteur_thread != (NB_THREADS - 1))
    {
        pthread_cond_wait(&tlm_rep, &mutex_aff);
    }
    printf("resultat_global = %d\n", resultat_global);
    pthread_mutex_unlock(&mutex_aff);
}

int main(int argc, char *argv[])
{
    pthread_t thread[NB_THREADS];
    pthread_attr_t attr, attr_aff;
    int rc, rc_aff, t;
    void *status;

    int indice_debut_vecteur[4];
    indice_debut_vecteur[0] = 0;
    indice_debut_vecteur[1] = 100;
    indice_debut_vecteur[2] = 200;
    indice_debut_vecteur[3] = 300;

    init_vecteur(vecteur1);
    init_vecteur(vecteur2);

    /* Initialisation et activation d’attributs */
    pthread_attr_init(&attr);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    pthread_attr_init(&attr_aff);                                    //valeur par défaut
    pthread_attr_setdetachstate(&attr_aff, PTHREAD_CREATE_JOINABLE); //attente du thread possible

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutex_aff, NULL);
    pthread_cond_init(&tlm_rep, NULL);

    for (int t = 0; t < (NB_THREADS - 1); t++)
    {
        printf("Creation du thread %d\n", t);
        rc = pthread_create(&thread[t], &attr, produit_scalaire, &indice_debut_vecteur[t]);
        if (rc)
        {
            printf("ERROR; le code de retour de pthread_create() est %d\n", rc);
            exit(-1);
        }
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

    return 0;
}