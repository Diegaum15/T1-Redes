#ifndef JANELA_H
#define JANELA_H
#include "protocolo.h"
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>

// Desliza uma janela, passa quem sera o ultimo da janela
void desliza_janela(protocolo *janela[MAX_JANELA], protocolo *next);

// Função que envia pedido e recebe lista de arquivos disponíveis no servidor
void pede_e_recebe_lista(int socket);

void pede_e_recebe_video(int socket, const char *nome_video);

// Funcao que abre um video
void abre_video(const char *filename);

// Funcao para listar arquivos de vídeo no diretório
void lista_arquivos(int socket);

// Função para enviar um arquivo ao cliente
void envia_arquivo(int socket, const char *filename);

// Função para mandar os arquivos em partes
void envia_partes_arquivo(int socket, uint8_t seq_inicial, const char *arquivo);

// Função que processa a mensagem do cliente
void processa_mensagem_cliente(int socket, protocolo *msg);

void pede_e_recebe_video(int socket, const char *nome_video);

// Funcao que lida com o possiveis erros
void cuidar_erro(protocolo *msg);

// Aloca lista de strings
char **cria_lista(int tamanho);

// Libera lista de strings
void exclui_lista(char **lista, int tamanho);

// Libera ponteiros da janela
void exclui_janela(protocolo **janela);
#endif  // JANELA_H
