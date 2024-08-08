#ifndef JANELA_H
#define JANELA_H
#include "protocolo.h"
#include <sys/stat.h>

// Desliza uma janela, passa quem sera o ultimo da janela
void desliza_janela(protocolo *janela[MAX_JANELA], protocolo *next);

// Funcao que abre um video
void abre_video(const char *filename);

// Funcao que lida com o possiveis erros
void cuidar_erro(protocolo *msg);

// Aloca lista de strings
char **cria_lista(int tamanho);

// Libera lista de strings
void exclui_lista(char **lista, int tamanho);

// Libera ponteiros da janela
void exclui_janela(protocolo **janela);

#endif  // JANELA_H
