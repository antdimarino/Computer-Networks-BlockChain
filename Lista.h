#include <stdio.h>
#include <stdlib.h>

typedef struct Transazione{
    char ipMittente[16];
    int portaMittente;
    int credito;
    char ipDestinatario[16];
    int portaDestinatario;
    int numRandom;
} transazione;

struct temp{
    int n;
    int tempo;
    struct Transazione ts;
};

typedef struct Blocco{
    int n;
    int tempo;
    struct Transazione ts;
    struct Blocco *next;
} blocco;

void inserimentoCoda(blocco temp, blocco *genesi);
blocco* getBlocco(int i, blocco* genesi);
void stampaLista(blocco* genesi);

void inserimentoCoda(struct temp t, blocco *genesi)
{
    blocco *bl = malloc(sizeof(blocco));
    blocco *coda = malloc(sizeof(blocco));
    coda = genesi;
    bl->n = t.n;
    bl->tempo = t.tempo;
    bl->ts = t.ts;
    bl->next = NULL;

    while(coda->next != NULL)
        coda = coda->next;

    coda->next = bl;
   
}

blocco* getBlocco(int i, blocco* genesi)
{
    blocco* temp = genesi;
    int j;

    for(j = 0; j<i; j++)
    {
        if(temp->next == NULL)
        {
            temp->n = -1;
            return temp;
        }
        temp = temp->next;
    }

    return temp;
}

void stampaLista(blocco* genesi)
{
    blocco *temp = malloc(sizeof(blocco));
    temp = genesi;

    printf("STAMPA LISTA\n");

    while(temp != NULL)
    {
        printf("n = %d\n", temp->n);
        temp = temp->next;
    }

    free(temp);
}

// transazione getTransazione(int id)
// {
//     blocco* temp = genesi->next;
//     int i;

//     for(i = 0; i<size; i++)
//     {
//         if(temp->ts.numRandom != id)
//             temp = temp->next;
//         else
//             return temp->ts;        
//     }
// }
