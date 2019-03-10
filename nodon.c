#include "Lista.h"
#include "wrapper.h"

mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

blocco* genesi;
int size = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *disp;

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

    file = open("blocchi_nodon.txt", O_CREAT | O_RDONLY, mode);

    if( (fstat(file, &sb)) < 0)
    {
        perror("fstat error\n");
        exit(1);
    }

    sem_unlink("disp");
    if ( (disp = sem_open("disp", O_CREAT | O_EXCL, mode, 0)) == SEM_FAILED)
    {
        perror("inizializzazione semaforo fallita\n");
        exit(1);
    }
    
    numBlocchi = sb.st_size / sizeof(struct temp);

    for(i = 0; i<numBlocchi; i++)
    {
        read(file, &t, sizeof(struct temp));
        inserimentoCoda(t, genesi);
        size++;
    }

    close(file);

    printf("NODON: Numero blocchi presi dal file: %d\n", size);
    stampaLista(genesi);        

    //NODON PRONTO PER LAVORARE A PIENO REGIME

    //CREAZIONE THREAD CHE GENERA LA BLOCKCHAIN
    if( (pthread_create(&tid, NULL, produci, NULL)) < 0) 
    {
        perror("could not create thread");
        return 1;
    }

    Bind(socket, (struct sockaddr *) &serv_add, sizeof(serv_add));
    Listen(socket, 1);

    while(1)
    {
        conn_fd = Accept(socket, NULL, NULL);

        printf("NODON: Connessione accettata\n");
        FullRead(conn_fd, &indice, sizeof(int));
        printf("NODON: il BlockServer ha richiesto i blocchi dall'ultimo indice %d\n", indice);

        if( (bl = getBlocco(indice, genesi)) == NULL)
        {
            printf("NODON: L'ultimo blocco che il BlockServer dice di possedere, non esiste.\n");
            close(conn_fd);
            continue;                //ATTENZIONE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! DA VERIVICARE IN POSSESSO DEL BLOCKSERVER!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        }
       
        while(1)
        {   
            if(indice < numBlocchi) //Se il blocco da inviare al blockserver si trovava nel file
            {
                bl = bl->next; 
		        //allora si aspetta il tempo prima di inviare il blocco
                sleep(bl->tempo);
            }
            else
            {   //altrimenti se il blocco deve essere creato , allora l'attesa verra' fatta prima di inserire il nuovo blocco nella Blockchain.
                pthread_mutex_lock(&mutex);
                if(indice == size)
                {
                    pthread_mutex_unlock(&mutex);
                    //si attende che il thread crei un nuovo nodo e lo inserisca nella Blockchain...
                    sem_wait(disp);
                }
                else
                    pthread_mutex_unlock(&mutex); 
	            bl = bl->next;
            }
            FullWrite(conn_fd, &bl, sizeof(blocco));
            indice++;   

            nread = read(conn_fd, &check, sizeof(int));
            if(  check == 0 || nread == 0 ) 
            {  
                 //SE il server ha interrotto la comunicazione si torna indietro.
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
            if( (rand()%100)<50 )
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
        pthread_mutex_lock(&mutex);        
        sleep(t.tempo);
        inserimentoCoda(t, genesi);
        size++;
        FullWrite(file, &t, sizeof(struct temp));
        pthread_mutex_unlock(&mutex);
        sem_getvalue(disp,&semVal);
        if(semVal==0)
            sem_post(disp);
        printf("THREAD NODON: Blocco creato ed inserito\n");
    }
}
