#include "Lista.h"
#include "wrapper.h"

mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

blocco* genesi;
int size = 0;
char ip[16];
int terminato;
int pid;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ter = PTHREAD_MUTEX_INITIALIZER;
pthread_t tidOttieniNodi;
pthread_attr_t att;
int numBlocchi;
int enable = 1;

void* ottieniNodi(void* arg);
void* gestoreClient(void* arg);
int sommaCredito();
int sommaTransazioni(char ip[], int porta);
void signalHandler(int segnaleRicevuto);

int main(int argc, char* argv[])
{
    int file, i = 0;
    struct stat sb;
    struct temp t;
    int dimAr = 1;

    int list_fd;
    int *client = (int *)calloc(dimAr, sizeof(int));
    struct sockaddr_in server;

    pthread_t *tid = (pthread_t*)calloc(dimAr, sizeof(pthread_t));
    pthread_attr_init(&att);
    pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED);

    if(argc < 2)
    {
        printf("Input Error: add an address");
        exit(1);        
    }
    pid = getpid();

    strcpy(ip, argv[1]);

    genesi = malloc(sizeof(blocco));
    genesi->n = 0;
    genesi->tempo = 0;
    genesi->next = NULL;
 
    list_fd = Socket(AF_INET, SOCK_STREAM, 0); 

    setsockopt(list_fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
    setsockopt(list_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(2000);

    file = open("blocchi_blockserver.txt", O_CREAT | O_RDONLY, mode);

    if( (fstat(file, &sb)) < 0)
    {
        printf("fstat error\n");
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

    printf("BLOCKSERVER: Numero blocchi presi dal file: %d\n", size);
    stampaLista(genesi); 

    if( (pthread_create(&tidOttieniNodi, NULL, ottieniNodi, (void *) &numBlocchi)) < 0) 
    {
        printf("could not create thread");
        return 1;
    }

    Bind(list_fd, (struct sockaddr *) &server, sizeof(server));
    Listen(list_fd, 1024);

    i=0;
    while(1)
    {    
        signal(SIGUSR1, signalHandler);
        client[i] = Accept(list_fd, NULL, NULL);
        
        printf("Connessione accettata\n");

        if( (pthread_create(&tid[i], &att, gestoreClient, (void* ) &client[i]) )  < 0) 
        {
            printf("could not create thread");
            return 1;
        }

        i++;
        if(i == dimAr )
        {
            if( (client = (int *)realloc(client, (i+5)*sizeof(int))) == NULL)
            {
                printf("Memoria insufficiente");
                exit(1);
            }                

            if( (tid = (pthread_t*)realloc(tid, (i+5)*sizeof(pthread_t))) == NULL)
            {
                printf("Memoria insufficiente");
                exit(1);
            }

            dimAr += 5;
        }
        
    }
    return 0;
}

void* gestoreClient(void* arg)
{
    int sock = *(int *) arg;

    int scelta;
    int n;
    int i;
    struct temp t;
    blocco* blTemp;
    char ip[16];
    int porta;
    int sum;
    int check;
    int nread, count;

    do
    {
        if( (FullRead(sock, &scelta, sizeof(int))) == -1)
        {
            printf("THREAD GESTORE-CLIENT: Connessione persa\n");
            pthread_exit(NULL);
        }

        switch (scelta)
        {
            case 1:
                if( FullRead(sock, &n, sizeof(int)) == -1 )
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }

                pthread_mutex_lock(&mutex);
                if( n <= size)
                {
                    sum =  1 + size - n;  

                    FullWrite(sock, &n, sizeof(int)); 

                    for(i = sum; i <= size ; i++)
                    {

                        blTemp = getBlocco(i, genesi);
                        t.n = blTemp->n;
                        t.tempo = blTemp->tempo;
                        t.ts = blTemp->ts;

                        FullWrite(sock, &t, sizeof(struct temp));            
                    }       
                    pthread_mutex_unlock(&mutex);  
                }
                else
                {
                    pthread_mutex_unlock(&mutex); 
                    n = -1;
                    FullWrite(sock, &n, sizeof(int));                
                }
                
                break;

            case 2:
                if( FullRead(sock, &n, sizeof(int)) == -1)
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }

                if( ( blTemp = getBlocco(n, genesi) ) == NULL)
                {
                    n = -1;
                    FullWrite(sock, &n, sizeof(int));
                    break;
                }
                else
                {
                    n = 1;
                    FullWrite(sock, &n, sizeof(int));
                }
                
                t.n = blTemp->n;
                t.tempo = blTemp->tempo;
                t.ts = blTemp->ts;

                FullWrite(sock, &t, sizeof(struct temp)); 
                break;

            case 3:
                sum = sommaCredito();

                FullWrite(sock, &sum, sizeof(int));            
                break;

            case 4:

                if( FullRead(sock, &ip, sizeof(ip)) == -1)
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }

                if( FullRead(sock, &porta, sizeof(int)) == -1)
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }

                sum = sommaTransazioni(ip, porta);

                FullWrite(sock, &sum, sizeof(int));   

                break;
            case 5:
                if ( FullRead(sock, &ip, sizeof(ip)) == -1)
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }

                if ( FullRead(sock, &porta, sizeof(int)) == -1)
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }
                    
                blTemp = genesi;
                pthread_mutex_lock(&mutex);
                while(blTemp->next != NULL)
                {  
                    blTemp = blTemp->next;
                    if((strcmp(blTemp->ts.ipDestinatario, ip) == 0 && blTemp->ts.portaDestinatario == porta) || (strcmp(blTemp->ts.ipMittente, ip) == 0 && blTemp->ts.portaMittente == porta) )
                    {
                        n = 1;

                        FullWrite(sock, &n, sizeof(int));

                        t.n = blTemp->n;
                        t.tempo = blTemp->tempo;
                        t.ts = blTemp->ts;

                        FullWrite(sock, &t, sizeof(struct temp));

                        if( FullRead(sock, &i, sizeof(int)) == -1)
                        {
                            printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                            pthread_exit(NULL);
                        }
                    }
                }
                pthread_mutex_unlock(&mutex);

    
                n = 0;
                FullWrite(sock,&n,sizeof(int));
                break;

            case 6:
				if ( FullRead(sock, &ip, sizeof(ip)) == -1)
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }

                if ( FullRead(sock, &porta, sizeof(int)) == -1)
                {
                    printf("THREAD GESTORE-CLIENT: Connessione persa\n");
                    pthread_exit(NULL);
                }

                sum = 0;
                count = 0;
                blTemp = genesi;
                pthread_mutex_lock(&mutex);
               	while(blTemp->next != NULL)
                {
                    blTemp = blTemp->next;

                    if( strcmp(blTemp->ts.ipMittente, ip) == 0 && blTemp->ts.portaMittente == porta )
                    {
                        sum-=blTemp->ts.credito;
                        count++;
                    }
                    else if( strcmp(blTemp->ts.ipDestinatario, ip) == 0 && blTemp->ts.portaDestinatario == porta )
                    {
                        sum += blTemp->ts.credito;	
                        count++;
                    }
                }
                pthread_mutex_unlock(&mutex);

                if(count > 0)
                {
                    FullWrite(sock, &count, sizeof(int));
                
                    FullWrite(sock, &sum, sizeof(int));
                }
                else
                    FullWrite(sock, &count, sizeof(int));
                

				break;            

            default:

                printf("Opzione non prevista");

                break;
        }

        if( FullRead(sock, &check, sizeof(int)) == -1)
        {
            printf("THREAD GESTORE-CLIENT: Connessione persa\n");
            pthread_exit(NULL);
        }

    }while(check == 1);

    pthread_exit(NULL);
}

void* ottieniNodi(void * arg)
{
    int numBl = *(int *) arg;
    int socket, len;
    struct sockaddr_in clientNodon;
    struct temp t;
    int file;
    int check = 1;

    file = open("blocchi_blockserver.txt", O_CREAT | O_WRONLY, mode);
    lseek(file, 0, SEEK_END);


    socket = Socket(AF_INET, SOCK_STREAM, 0);

    setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(int));
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

    clientNodon.sin_family = AF_INET;
    clientNodon.sin_port = htons(1025);

    if ( (inet_pton(AF_INET, ip, &clientNodon.sin_addr)) <= 0) {
		printf("Address creation error");
		return NULL;
    }

    len = sizeof(clientNodon);

    while( (Connect(socket, (struct sockaddr *)&clientNodon, len)) == -1)
    {
        printf("Nuovo tentativo di riconnessione tra 10 secondi\n");
        sleep(10);
    }



    printf("BLOCKSERVER: Chiedo al nodon i blocchi dall'indice: %d\n", numBl);
    FullWrite(socket, &numBl, sizeof(int));

    while( FullRead(socket, &t, sizeof(struct temp)) != -1 )
    {       
        if (t.n == size+1)
        {
            pthread_mutex_lock(&mutex);
            inserimentoCoda(t, genesi);        
            size++;
            write(file, &t, sizeof(struct temp));
            printf("THREAD BLOCKSERVER: Blocco ricevuto ed inserito: %d.\n\n", t.n);
            pthread_mutex_unlock(&mutex);   

            printf("n = %d\ntempo = %d\nIp Destinatario: %s\t Porta: %d\n", t.n,t.tempo, t.ts.ipDestinatario, t.ts.portaDestinatario);
            printf("Ip Mittente: %s\t Porta Mittente: %d\nCredito: %d\nNumero Randomico: %d\n\n\n\n", t.ts.ipMittente, t.ts.portaMittente, t.ts.credito, t.ts.numRandom);        
        }

        FullWrite(socket, &check, sizeof(int));         
    }
    printf("THREAD OTTIENINODI: Ho perso la connessione con nodon\n");


    kill(pid, SIGUSR1);
    pthread_exit(NULL);    
}

int sommaCredito()
{
    blocco *bl = genesi;
    int sum = 0;

    pthread_mutex_lock(&mutex);
    if(bl != NULL)
    {
        bl = bl->next;
        while(bl != NULL)
        {
            sum = sum + bl->ts.credito;
            bl = bl->next;
        }
    }
    pthread_mutex_unlock(&mutex);

    return sum;    
}

int sommaTransazioni(char ip[], int porta)
{
    blocco *bl = genesi;
    int sum = 0;

    pthread_mutex_lock(&mutex);
    while( bl != NULL)
    {
        if( (strcmp(bl->ts.ipDestinatario, ip) == 0  && bl->ts.portaDestinatario == porta)  || (strcmp(bl->ts.ipMittente, ip) == 0 && bl->ts.portaMittente == porta) )
            sum++;        
        bl = bl->next;
    }
    pthread_mutex_unlock(&mutex);
    return sum;
}

void signalHandler(int segnaleRicevuto)
{
    pthread_join(tidOttieniNodi, NULL);


    if( (pthread_create(&tidOttieniNodi, NULL, ottieniNodi, (void *) &size)) < 0) 
    {
        printf("could not create thread");
    }
}
