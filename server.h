#ifndef SERVER_H
#define SERVER_H
#include "janela.h"

// Função que processa a mensagem do cliente
void processa_mensagem_cliente(int socket, protocolo *msg);

// Função que processa um pedido de vídeo do cliente
void processa_pedido_video(int socket, const char *filename);

// Funcao para listar arquivos de vídeo no diretório
void lista_arquivos(int socket);


#endif  // SERVER_H
