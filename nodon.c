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

    Bind(socket, (struct sockaddr *) &serv_add, sizeof(serv_add));
    Listen(socket, 1);

    len = sizeof(client);

    conn_fd = Accept(socket, (struct sockaddr* ) &client, len);

    printf("NODON: Connessione accettata\n");

    while(1)
    {
        FullRead(conn_fd, &indice, sizeof(int));

        printf("NODON: il BlockServer ha richiesto i blocchi dall'ultimo indice %d\n", indice);

        //sezione critica?
        diff = size-indice;
        FullWrite(conn_fd, &diff, sizeof(int));
        
        bl = getBlocco(indice+1, size, genesi);
        // 
        for(i = 0; i<diff; i++)
        {
            FullWrite(conn_fd, &bl, sizeof(blocco));
            bl = bl->next;
        }

    }
    return 0;
}
