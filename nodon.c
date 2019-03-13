#include "Lista.h"
#include "wrapper.h"

mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

blocco* genesi;
int size = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *disp;
int bloccato = 0;
int enable = 1;

void *produci(void *arg);

int main(int argc, char* argv[])
{
    int file, numBlocchi, i;
    struct stat sb;
    blocco temp;
    struct temp t;

    struct sockaddr_in serv_add;
    int socket, conn_fd;
    int len;
    int indice;
    blocco* bl;
    int diff;
    int check;
    int nread = 0;
    pthread_t tid;

    sem_unlink("disp");
    if ( (disp = sem_open("disp", O_CREAT | O_EXCL, mode, 0)) == SEM_FAILED)
    {
        perror("inizializzazione semaforo fallita\n");
        exit(1);
    }

    size = 0;
    genesi = malloc(sizeof(blocco));
    genesi->n = 0;
    genesi->tempo = 0;
    genesi->next = NULL;

    socket = Socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    serv_add.sin_family      = AF_INET;
    serv_add.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_add.sin_port        = htons(1025);

    file = open("blocchi_nodon.txt", O_CREAT | O_RDONLY, mode);

    if( (fstat(file, &sb)) < 0)
    {
        perror("fstat error\n");
        exit(1);
    }
    
    numBlocchi = sb.st_size / sizeof(struct temp);

    for(i = 0; i<numBlocchi; i++)
    {
       if( (FullRead(file, &t, sizeof(struct temp))) == -1)
       {
           printf("NODON: Errore sulla lettura del file\n");
           exit(1);
       }         
        inserimentoCoda(t, genesi);
        size++;
    }

    close(file);

    printf("NODON: Numero blocchi presi dal file: %d\n", size);
    stampaLista(genesi);        

    if( (pthread_create(&tid, NULL, produci, NULL)) < 0) 
    {
        perror("could not create thread");
        return 1;
    }

    Bind(socket, (struct sockaddr *) &serv_add, sizeof(serv_add));
    Listen(socket, 1);

    while(1)
    {
        printf("NODON: Mi metto in attesa di una nuova connessione\n");
        conn_fd = Accept(socket, NULL, NULL);

        printf("NODON: Connessione accettata\n");
        if( ( FullRead(conn_fd, &indice, sizeof(int)) ) == -1)
        {
            printf("NODON: Connessione persa\n");
            break;
        }

        printf("NODON: il BlockServer ha richiesto i blocchi dall'ultimo indice %d\n", indice);

        if( (bl = getBlocco(indice, genesi)) == NULL)
        {
            printf("NODON: L'ultimo blocco che il BlockServer dice di possedere, non esiste.\n");
            close(conn_fd);
            continue;                
        }
       
        while(1)
        {   

            if(indice < numBlocchi) 
            {
                printf("NODON: Voglio ricevere i blocchi dal file\n");
                bl = bl->next; 
		        
                sleep(bl->tempo);
            }
            else
            {   
                pthread_mutex_lock(&mutex);
                if(indice == size)
                {
                    bloccato = 1;
                    pthread_mutex_unlock(&mutex);
                    sem_wait(disp);
                }
                else
                    pthread_mutex_unlock(&mutex); 

	            bl = bl->next;
            }
            t.n = bl->n;
            t.tempo = bl->tempo;
            t.ts = bl->ts;

            FullWrite(conn_fd, &t, sizeof(struct temp));
            printf("NODON: blocco inviato numero: %d\ttempo: %d\n\n", t.n, t.tempo);
            indice++;


            nread = FullRead(conn_fd, &check, sizeof(int));
            if(  check != 1 || nread == -1 ) 
            {  
                printf("NODON: Il BlockServer ha interrotto la connessione!\n");
                close(conn_fd);
                break;
            }  
        }
    }

    sem_close(disp);
    sem_unlink("disp");
    return 0;
}

void *produci(void* arg)
{
    int semVal=1;
    int inizio=1;
    int file;
    time_t temp=time(NULL);
    struct temp t;

    srand(time(NULL));

    snprintf(t.ts.ipMittente, 16, "%d.%d.%d.%d", rand()%256, rand()%256, rand()%256, rand()%256);
    t.ts.portaMittente = 1024 + rand()%64512;
    snprintf(t.ts.ipDestinatario, 16, "%d.%d.%d.%d", rand()%256, rand()%256, rand()%256, rand()%256);
    t.ts.portaDestinatario = 1024 + rand()%64512;	

    printf("THREAD NODON: Attivo\n");

    file = open("blocchi_nodon.txt", O_CREAT | O_WRONLY, mode);
    lseek(file, 0, SEEK_END);

    while(1)
    {
        t.n = size+1;
        t.tempo = 5 + rand()%11;

        if( (time(NULL)-temp) > 20 )
        {
            temp=time(NULL);
            if( (rand()%1) == 0 )
            {
                snprintf(t.ts.ipMittente, 16, "%d.%d.%d.%d", rand()%256, rand()%256, rand()%256, rand()%256);
                t.ts.portaMittente = 1024 + rand()%64512;	
            }
            else
            {
                snprintf(t.ts.ipDestinatario, 16, "%d.%d.%d.%d", rand()%256, rand()%256, rand()%256, rand()%256);
                t.ts.portaDestinatario = 1024 + rand()%64512;
            }
	    }

        t.ts.credito = 1 + rand()%1000000;
        t.ts.numRandom = 1 + rand()%999999;

        sleep(t.tempo);

        pthread_mutex_lock(&mutex); 

        inserimentoCoda(t, genesi);
        size++;
        FullWrite(file, &t, sizeof(struct temp));
        printf("THREAD NODON: Blocco creato ed inserito n: %d\n\n", t.n);

        if(bloccato == 1)
        {
            bloccato = 0;
            pthread_mutex_unlock(&mutex);
            sem_post(disp);
            printf("THREAD NODON: Ho svegliato il padre\n\n");
        }
        else
            pthread_mutex_unlock(&mutex);
        
    }
}
