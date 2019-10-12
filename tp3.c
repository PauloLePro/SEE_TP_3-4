//gcc -o tp3 tp3.c -lpthread

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define NB_THREADS 3

void *travailUtile(void *args)
{
   int i;
   double resultat = 0.0;

   int *pargs = args;

   for (i = 0; i < 100; i++)
   {
      resultat = resultat + (double)random();
   }

   printf("avant add resultat = %e\n", resultat);

   if (pargs != NULL)
   {
      resultat += *pargs;
   }

   printf("resultat = %e\n", resultat);
   pthread_exit((void *)0);
}

int main(int argc, char *argv[])
{
   pthread_t thread[NB_THREADS];
   pthread_attr_t attr;
   char chaine[50];

   double args0 = 1000000000000000;
   double args1 = 2000000000000000;
   double args2 = 3000000000000000;

   int rc, t;
   void *status;

   size_t stksize;
   size_t rstksize = 9412608;

   for (t = 0; t < NB_THREADS; t++)
   {

      sprintf(chaine, "args%d", t);

      /* Initialisation et activation d’attributs */
      pthread_attr_init(&attr);                                    //valeur par défaut
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); //attente du thread possible

      if (pthread_attr_getstacksize(&attr, &stksize) != 0)
      {
         perror("pthread_attr_getstacksize \n");
      }
      else
      {
         printf("Thread(%d) ==> taille de la stack %d ko \n", t, (stksize / 1024));
      }

      if (pthread_attr_setstacksize(&attr, rstksize) != 0)
      {
         perror("pthread_attr_getstacksize \n");
      }
      else
      {
         printf("pthread_attr_setstacksize \n");
         if (pthread_attr_getstacksize(&attr, &stksize) != 0)
         {
            perror("pthread_attr_getstacksize \n");
         }
         else
         {
            printf("Thread(%d) ==> taille de la stack %d ko \n", t, (stksize / 1024));
         }
      }

      printf("Creation du thread %d\n", t);
      rc = pthread_create(&thread[t], &attr, travailUtile, &chaine);
      if (rc)
      {
         printf("ERROR; le code de retour de pthread_create() est %d\n", rc);
         exit(-1);
      }
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
}
