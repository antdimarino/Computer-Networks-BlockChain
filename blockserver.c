#include "Lista.h"
#include "wrapper.h"

mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

blocco* genesi;
int size = 0;
char ip[16];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* ottieniNodi(void* arg);
void* gestoreClient(void* arg);

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
        perror("Input Error: add an address\n");
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
    int socket = *(int *) arg;


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