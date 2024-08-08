#ifndef CLIENT_H
#define CLIENT_H
#include "janela.h"

// Função que envia pedido e recebe lista de arquivos disponíveis no servidor
void pede_e_recebe_lista(int socket);

// Função que envia pedido e recebe um vídeo do servidor
void pede_e_recebe_video(int socket, const char *nome_video);

#endif  // CLIENT_H