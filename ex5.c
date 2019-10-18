#include <aio.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s {filename}\n", argv[0]);
        return -1;
    }

    struct aiocb cb; // bloc de contrôle de l’E/S asynchrone
    const struct aiocb *cbs[1];

    //Ouverture du fichier (spécifié en argument) sur lequel l’E/S va être effectuée
    FILE *file = fopen(argv[1], "r+");
    if (file == NULL)
    {
        perror("error fopen :");
    }

    //definition du bloc de contrôle de l’entrée/sortie
    //memset(&cb, 0, sizeof(struct aiocb));
    cb.aio_buf = malloc(11);
    cb.aio_fildes = fileno(file); //récupérer le descripteur d’un fichier à partir de son nom
    cb.aio_nbytes = 10;
    cb.aio_offset = 0;
    cb.aio_reqprio = 0;

    //lancer la lecture
    if (aio_read(&cb) == -1)
    {
        perror("error aio_read :");
    }

    //Suspension du processus dans l’attente de la terminaison de la lecture
    cbs[0] = &cb;
    if (aio_suspend(cbs, 1, NULL) == -1)
    {
        perror("error aio_suspend :");
    }

    if (aio_return(&cb) == -1)
    {
        perror("error aio_return :");
    }
    else
    {
        printf("operation AIO a retouree %d\n", aio_return(&cb));
    }

    fclose(file);

    return 0;
}