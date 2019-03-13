#include "Lista.h"
#include "wrapper.h"

int main(int argc, char* argv[])
{
    int sock;
    struct sockaddr_in serv_add;

    int scelta = 0;
    int n;
    struct temp t;
    char ip[16];
    int porta;
    int i;
    int check;

    if(argc <2)
    {
        perror("Input error: add an address");
        exit(1);
    }

    sock = Socket(AF_INET, SOCK_STREAM, 0);

    serv_add.sin_family = AF_INET;
    serv_add.sin_port = htons(2000);

    if ( (inet_pton(AF_INET, argv[1], &serv_add.sin_addr)) <= 0) {
		perror("Address creation error");
		return 1;
    }

    Connect(sock, (struct sockaddr *) &serv_add, sizeof(serv_add));

    do
    {
        printf("BLOCK CLIENT: Scegli un operazione da effettuare sull'attuale blockchain\n");
        printf("[1] Visualizzare le ultime n transazioni\n[2] Visualizzare una generica transazione\n");
        printf("[3] Visualizzare la somma dei valori di tutta la blockchain\n[4] Cercare il numero di transazioni di un indirizzo specifico\n[5] Cercare tutte le transazioni di un indirizzo specifico\n");
        printf("Scelta: ");
        scanf("%d", &scelta);
        printf("\n");

        FullWrite(sock, &scelta, sizeof(int));

        switch (scelta)
        {
            case 1:
                printf("Da quale identificativo vuoi visualizzare le transazioni?");
                printf("Scelta: ");
                scanf("%d", &n);
                printf("\n");

                FullWrite(sock, &n, sizeof(int));

                FullRead(sock, &n, sizeof(int));

                if( n == -1 )
                    printf("Questo blocco non e presente nella blockchain\n");
            
                for(i = 0; i<n; i++)
                {
                    FullRead(sock, &t, sizeof(struct temp));

                    printf("n = %d\ntempo = %d\nIp Destinatario: %s\t Porta: %d\n", t.n,t.tempo, t.ts.ipDestinatario, t.ts.portaDestinatario);
                    printf("Ip Mittente: %s\t Porta Mittente: %d\nCredito: %d\nNumero Randomico: %d\n\n\n\n", t.ts.ipMittente, t.ts.portaMittente, t.ts.credito, t.ts.numRandom);
                }

                break;
            case 2:
                printf("Inserisci l'identificativo del blocco di cui vuoi visualizzare la transazione\n");
                printf("Scelta: ");
                scanf("%d", &n);
                printf("\n");

                FullWrite(sock, &n, sizeof(int));

                FullRead(sock, &t, sizeof(struct temp));

                printf("n = %d\ntempo = %d\nIp Destinatario: %s\t Porta: %d\n", t.n,t.tempo, t.ts.ipDestinatario, t.ts.portaDestinatario);
                printf("Ip Mittente: %s\t Porta Mittente: %d\nCredito: %d\nNumero Randomico: %d\n\n\n\n", t.ts.ipMittente, t.ts.portaMittente, t.ts.credito, t.ts.numRandom);
                break;
            case 3:
                FullRead(sock, &n, sizeof(int));
                printf("La somma dei valori di tutte le attuali transazioni = %d\n", n);                
                break;
            case 4:
                printf("Inserisci indirizzo IP: ");
                scanf("%s", ip);
                printf("\nInserisci la porta: ");
                scanf("%d", &porta);
                printf("\n");

                FullWrite(sock, &ip, sizeof(ip));

                FullWrite(sock, &porta, sizeof(int));

                FullRead(sock, &n, sizeof(int));

                printf("L'indirizzo %s: %d e' coinvolto in %d transazioni\n", ip, porta, n);

                break;
            case 5:
		printf("Inserire l'indirizzo IP: ");
                scanf("%s", ip);
                printf("\nInserire la porta: ");
                scanf("%d", &porta);
                printf("\n");
                FullWrite(sock, &ip, sizeof(ip));
	 	FullWrite(sock, &porta, sizeof(int));
		 while(FullRead(sock,&n,sizeof(int))!=-1)
		 {
		  if(n==0)
		    break;
		  if ( FullRead(sock,&t,sizeof(struct temp))==-1)
		  {
		    printf("Connessione con il Blockserver interrotta\n");
		    break;
	          }
		  i=1;
		  FullWrite(sock,&i,sizeof(int));
printf("\nTransazione numero= %d\nIp Mittente= %s\t\tporta Mittente= %d\nIp Destinatario= %s\t\tporta Destinatario= %d\nAmmontare= %d\nNumero random= %d\n",t.n,t.ts.ipMittente,t.ts.portaMittente,t.ts.ipDestinatario,t.ts.portaDestinatario,t.ts.credito,t.ts.numRandom);
		 }
		  printf("\nRichiesta conclusa.\n");		
                break;
            default:
                printf("Opzione non prevista\n");
                break;
        }

        printf("[1] Continui ad usufruire del servizio\n[0]Esci\nScelta: ");
        scanf("%d", &check);
        printf("\n");

        FullWrite(sock, &check, sizeof(int));

    }while (check == 1);

    return 0;
}
