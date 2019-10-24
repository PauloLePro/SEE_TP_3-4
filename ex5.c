//gcc -o ex5 ex5.c -lrt ./ex5 fichasync.txt

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

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s {filename}\n", argv[0]);
        return -1;
    }

    int fd;
    struct aiocb cb, cb1, writecb; // bloc de contrôle de l’E/S asynchrone
    const struct aiocb *cbs[3];

    struct sigaction act;

    memset(&act, '\0', sizeof(act));

    //Ouverture du fichier (spécifié en argument) sur lequel l’E/S va être effectuée
    if ((fd = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
    {
        perror("error open");
        return -1;
    }

    act.sa_sigaction = &hdl;
    act.sa_flags = SA_SIGINFO;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGRTMIN, &act, NULL) < 0)
    {
        perror("sigaction");
        return -1;
    }

    //definition du bloc de contrôle de l’entrée/sortie
    if ((cb.aio_buf = malloc(11)) == NULL)
    {
        perror("malloc");
    }
    cb.aio_fildes = fd; //récupérer le descripteur d’un fichier à partir de son nom
    cb.aio_nbytes = 10;
    cb.aio_offset = 0;
    cb.aio_reqprio = 0;
    cb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    cb.aio_sigevent.sigev_signo = SIGRTMIN;
    cb.aio_sigevent.sigev_value.sival_int = 0;

    if ((cb1.aio_buf = malloc(11)) == NULL)
    {
        perror("malloc");
    }
    cb1.aio_fildes = fd; //récupérer le descripteur d’un fichier à partir de son nom
    cb1.aio_nbytes = 10;
    cb1.aio_offset = 0;
    cb1.aio_reqprio = 0;
    cb1.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    cb1.aio_sigevent.sigev_signo = SIGRTMIN;
    cb1.aio_sigevent.sigev_value.sival_int = 0;

    memset(&writecb, 0, sizeof(struct aiocb));
    writecb.aio_fildes = fd;
    writecb.aio_buf = "test\n";
    writecb.aio_nbytes = strlen("test\n");
    writecb.aio_offset = 0;
    writecb.aio_reqprio = 0;
    writecb.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
    writecb.aio_sigevent.sigev_signo = SIGRTMIN;
    writecb.aio_sigevent.sigev_value.sival_int = 1;

    //lancer la lecture
    if (aio_read(&cb) == -1)
    {
        perror("error aio_read :");
    }

    if (aio_write(&writecb) == -1)
    {
        perror("error aio_write :");
    }

    //lancer la lecture
    if (aio_read(&cb1) == -1)
    {
        perror("error aio_read :");
    }

    //Suspension du processus dans l’attente de la terminaison de la lecture
    cbs[0] = &cb;
    cbs[1] = &writecb;
    cbs[2] = &cb1;

    if (aio_suspend(cbs, 3, 0) == -1)
    {
        perror("error aio_suspend :");
    }

    if (aio_return(&cb) == -1)
    {
        perror("error aio_return :");
    }
    else
    {
        printf("(lecture) operation AIO a retouree %d\n", aio_return(&cb));
    }

    if (aio_return(&writecb) == -1)
    {
        perror("error aio_return :");
    }
    else
    {
        printf("(écriture) operation AIO a retouree %d\n", aio_return(&writecb));
    }

    if (aio_return(&cb1) == -1)
    {
        perror("error aio_return :");
    }
    else
    {
        printf("(lecture 2) operation AIO a retouree %d\n", aio_return(&cb1));
    }

    printf("(lecture) %s \n", cb.aio_buf);
    printf("(lecture 2) %s", cb1.aio_buf);

    close(fd);

    return 0;
}