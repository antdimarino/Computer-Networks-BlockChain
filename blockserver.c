#include "Lista.h"
#include "wrapper.h"

mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

blocco* genesi;
int size = 0;
char ip[16];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* ottieniNodi(void* arg);
void* gestoreClient(void* arg);
int sommaCredito(blocco *genesi);
int sommaTransazioni(char ip[], int porta, blocco* genesi);

int main(int argc, char* argv[])
{
    int file, numBlocchi, i;
    struct stat sb;
    struct temp t;

    int list_fd;
    int client[50];
    struct sockaddr_in server;

    pthread_t tid;

    if(argc < 2)
    {
        perror("Input Error: add an address");
        exit(1);        
    }

    strcpy(ip, argv[1]);

    size = 0;
    genesi = malloc(sizeof(blocco));
    genesi->n = 0;
    genesi->tempo = 0;
    genesi->next = NULL;

    //PREPARAZIONE SOCKET E SOCKADDR 
    list_fd = Socket(AF_INET, SOCK_STREAM, 0); 

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(2000);

    file = open("blocchi_blockserver.txt", O_CREAT | O_RDONLY, mode);

    if( (fstat(file, &sb)) < 0)
    {
        perror("fstat error\n");
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

    printf("BLOCKSERVER: Numero blocchi presi dal file: %d\n", size);
    stampaLista(genesi); 

    //BLOCKSERVER PUO LAVORARE A PIENO REGIME 

    if( (pthread_create(&tid, NULL, ottieniNodi, NULL)) < 0) 
    {
        perror("could not create thread");
        return 1;
    }

    Bind(list_fd, (struct sockaddr *) &server, sizeof(server));
    Listen(list_fd, 1024);

    while(1)
    {
        while( client[i] = Accept(list_fd, NULL, NULL) )
        {
            printf("Connessione accettata\n");

            if( (pthread_create(&tid, NULL, gestoreClient, (void* ) &client[i]) )  < 0) 
            {
                perror("could not create thread");
                return 1;
            }
        }
    }


    pthread_join(tid, NULL);

    return 0;
}

void* gestoreClient(void* arg)
{
    int sock = *(int *) arg;

    int scelta;
    int n;
    int i;
    struct temp t;
    blocco* temp;
    char ip[16];
    int porta;
    int sum;

    FullRead(sock, &scelta, sizeof(int));

    switch (scelta)
    {
        case 1:
            FullRead(sock, &n, sizeof(int));

            pthread_mutex_lock(&mutex);
            if( n <= size)
            {
                sum = 1 + (size - n);
                pthread_mutex_unlock(&mutex);    
                FullWrite(sock, &sum, sizeof(int));

                for(i = 0; i<sum; i++) //MUTUA ESCLUSIONEEEE
                {

                    temp = getBlocco(i+n, genesi);
                    t.n = temp->n;
                    t.tempo = temp->tempo;
                    t.ts = temp->ts;

                    FullWrite(sock, &t, sizeof(struct temp));                
                }        
            }
            else
            {
                n = -1;
                FullWrite(sock, &n, sizeof(int));                
            }
            break;

        case 2:
            FullRead(sock, &n, sizeof(int));

            temp = getBlocco(n, genesi);
            t.n = temp->n;
            t.tempo = temp->tempo;
            t.ts = temp->ts;

            FullWrite(sock, &t, sizeof(struct temp)); 
            break;

        case 3:
            sum = sommaCredito(genesi);

            FullWrite(sock, &sum, sizeof(int));            
            break;

        case 4:
            FullRead(sock, &ip, sizeof(ip));

            FullRead(sock, &porta, sizeof(int));

            sum = sommaTransazioni(ip, porta, genesi);

            FullWrite(sock, &sum, sizeof(int));   

            break;
    
        default:
            break;
    }

    pthread_exit(NULL);
}

void* ottieniNodi(void * arg)
{
    int socket, len;
    struct sockaddr_in clientNodon;
    struct temp t;
    int file;
    int check = 1;

    file = open("blocchi_blockserver.txt", O_CREAT | O_WRONLY, mode);
    lseek(file, 0, SEEK_END);


    socket = Socket(AF_INET, SOCK_STREAM, 0);

    clientNodon.sin_family = AF_INET;
    clientNodon.sin_port = htons(1025);

    if ( (inet_pton(AF_INET, ip, &clientNodon.sin_addr)) <= 0) {
		perror("Address creation error");
		return NULL;
    }

    len = sizeof(clientNodon);

    Connect(socket, (struct sockaddr *)&clientNodon, len);

    printf("BLOCKSERVER: Chiedo al nodon i blocchi dall'indice: %d\n", size);
    FullWrite(socket, &size, sizeof(int));

    while(1)
    {        
        FullRead(socket, &t, sizeof(struct temp));
        pthread_mutex_lock(&mutex);
        inserimentoCoda(t, genesi);        
        size++;
        write(file, &t, sizeof(struct temp));
        printf("THREAD BLOCKSERVER: Blocco ricevuto ed inserito: %d.\n\n", t.n);
        pthread_mutex_unlock(&mutex);   

        

        FullWrite(socket, &check, sizeof(int));         
    }


    pthread_exit(NULL);    
}

int sommaCredito(blocco* genesi)
{
    blocco *temp = malloc(sizeof(blocco));
    temp = genesi;
    int sum = 0;

    if(temp != NULL)
    {
        temp = temp->next;
        while(temp != NULL)
        {
            sum = sum + temp->ts.credito;
            temp = temp->next;
        }
    }

    return sum;    
}

int sommaTransazioni(char ip[], int porta, blocco* genesi)
{
    blocco *temp = malloc(sizeof(blocco));
    temp = genesi;
    int sum = 0;

    while( temp != NULL)
    {
        if( strcmp(temp->ts.ipDestinatario, ip) && temp->ts.portaDestinatario == porta)
            sum++;
        if( strcmp(temp->ts.ipMittente, ip) && temp->ts.portaMittente == porta)
            sum++;
        
        temp = temp->next;
    }
    return sum;
}
