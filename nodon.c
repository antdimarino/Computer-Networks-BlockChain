#include <sys/types.h>   /* predefined types */
#include <unistd.h>      /* include unix standard library */
#include <arpa/inet.h>   /* IP addresses conversion utiliites */
#include <sys/socket.h>  /* socket library */
#include <errno.h>	 /* include error codes */
#include <string.h>	 
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/select.h>
#include <semaphore.h>
#include "wrapper.h"
#include "Lista.h"
mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

blocco* genesi;
int size;
pthread_mutex_t *mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *sem;

void *produci(void *arg);

int main(int argc, char* argv[])
{
    int file numBlocchi, i;
    struct stat sb;
    blocco temp;

    struct sockaddr_in serv_add;
    int socket, conn_fd;
    int len;
    int indice;
    blocco* bl;
    int diff;
    int check;
    pthread_t tid;


    //INIZIALIZZAZIONE NODON
    size = 0;
    genesi = malloc(sizeof(blocco));
    genesi->n = 0;
    genesi->tempo = 0;
    genesi->next = NULL;

    socket = Socket(AF_INET, SOCK_STREAM, 0);

    serv_add.sin_family      = AF_INET;
    serv_add.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_add.sin_port        = htons(1025);

    file = open("blocchi_nodon.txt", O_CREAT | O_RWONLY, mode);
    if( (fstat(file, &sb)) < 0)
    {
        perror("Fstat error\n");
        exit(1);
    }

    sem_unlink("semaforo");
    if ( (sem = sem_open("semaforo", O_CREAT | O_EXCL, mode, 0)) == SEM_FAILED)
    {
        perror("inizializzazione semaforo fallita\n");
        exit(1);
    }
    
    numBlocchi = sb.st_size / sizeof(blocco);

    for(i = 0; i<numBlocchi; i++)
    {
        read(file, &temp, sizeof(blocco));
        inserimentoCoda(temp, genesi);
        size++;
    }

    printf("NODON: BlockChain Caricata\n");
    stampaLista(genesi);        

    //NODON PRONTO PER LAVORARE A PIENO REGIME

    //CREAZIONE THREAD CHE GENERA LA BLOCKCHAIN
    if( (pthread_create(&tid, NULL, produci, NULL) < 0)
    {
        perror("could not create thread");
        return 1;
    }

    Bind(socket, (struct sockaddr *) &serv_add, sizeof(serv_add));
    Listen(socket, 1);

    len = sizeof(client);

    while(1)
    {
        conn_fd = Accept(socket, (struct sockaddr* ) &client, len);

        printf("NODON: Connessione accettata\n");
        FullRead(conn_fd, &indice, sizeof(int));
        printf("NODON: il BlockServer ha richiesto i blocchi dall'ultimo indice %d\n", indice);

        bl = getBlocco(indice, size, genesi);

        while(1)
        {
            pthread_mutex_lock(mutex);
            if(indice == size)
            {
                //sincronizzati con il thread
                sem_wait(&semaforo);
                //aspetta che il nuovo blocco venga creato
            }  
            pthread_mutex_unlock(mutex); 

            bl = bl->next; 
            sleep(bl->tempo);
            FullWrite(conn_fd, &bl, sizeof(blocco));
            indice++;   

            if( read(conn_fd, &check, sizeof(int)) == 0 ) 
            {
                close(conn_fd);
                break;
            }
        }

    }

    sem_unlink("semaforo");
    sem_close(sem);
    return 0;
}

void *produci(void* arg)
{
    blocco prodotto;

    while(1)
    {
        prodotto.n = size+1;
        prodotto.tempo = 5 + rand()%11;
        prodotto.next = NULL;
        prodotto.ts.portaMittente = 1024 + rand()%64512;
        prodotto.ts.portaDestinatatio = 1024 + rand()%64512;
        prodotto.ts.credito = 1 + rand()%1000000;
        prodotto.ts.numRandom = 1 + rand()%999999;        
        snprintf(prodotto.ts.ipMittente, 16, "%03d.%03d.%03d.%03d", rand()%256, rand()%256, rand()%256, rand()%256);
        snprintf(prodotto.ts.ipDestinatario, 16, "%03d.%03d.%03d.%03d", rand()%256, rand()%256, rand()%256, rand()%256);

        sleep(prodotto.tempo);
        inserimentoCoda(prodotto, genesi);
    }
}
