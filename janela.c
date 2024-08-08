#include "janela.h"

void desliza_janela(protocolo *janela[MAX_JANELA], protocolo *next)
{
    exclui_msg(janela[0]);
    for (int i = 0; i < MAX_JANELA-1; i++){
        janela[i] = janela[i+1];
    }
    janela[MAX_JANELA-1] = next;
}


// Funcao que abre um video
void abre_video(const char *filename) 
{
    char comando[256];
    snprintf(comando, sizeof(comando), "xdg-open %s", filename);
    system(comando);
}


void cuidar_erro(protocolo *msg){
    if (msg->tipo == ERRO){
        switch (msg->seq)
        {
        case 1:
            //printf(ERRO_ACESSO_NEGADO);
            printf("Erro no acesso ao arquivo\n");
            exclui_msg(msg);
            // exit(1);
            break;
        case 2:
            printf(ERRO_NAO_ENCONTRADO);
            exclui_msg(msg);
            // exit(2);
            break;
        case 3:
            printf(ERRO_DISCO_CHEIO);
            exclui_msg(msg);
            // exit(3);
            break;
        default:
            printf(ERRO_PADRAO);
            exclui_msg(msg);
            // exit(4);
            break;
        }
    }
}

char **cria_lista(int tamanho){
    char **lista = malloc(tamanho * sizeof(char*));
    if (lista == NULL)
        return NULL;
    for(int i = 0; i < tamanho; i++){
        lista[i] = malloc(64 * sizeof(char));
        if (lista[i] == NULL)
            return NULL;
    }
    return lista;
}

void exclui_lista(char **lista, int tamanho){
    for(int i = 0; i < tamanho; i++){
        if (lista[i])
            free(lista[i]);
    }
    free(lista);
}

void exclui_janela2(protocolo **janela){
    for(int i = 0; i < MAX_JANELA; i++){
        if (janela[i])
            exclui_msg(janela[i]);
    }
}

void exclui_janela(protocolo *janela[]) {
    for (int i = 0; i < MAX_JANELA; i++) {
        if (janela[i] != NULL) {
            exclui_msg(janela[i]); // Certifique-se de que exclui_msg não está liberando memória já liberada.
            janela[i] = NULL;
        }
    }
}
