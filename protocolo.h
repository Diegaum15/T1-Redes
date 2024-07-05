#ifndef PROTOCOLO_H
#define PROTOCOLO_H

// Função para excluir uma mensagem
void exclui_msg(protocolo *msg);

// Função para alocar um vetor de tamanho tam
uint8_t* aloca_vetor(int tam);

// Função para alocar uma mensagem
protocolo* aloca_msg();

// Função para calcular a sequência
uint8_t cal_seq(protocolo *msg);

// Função para criar uma mensagem
protocolo* cria_msg(uint8_t seq, uint8_t tipo, const uint8_t *dados, size_t tam);

//espera resposta retorna 0 se receber antes de timeout, se nao retorna 1
//se timeout for NULL, espera indeterminadamente
int espera(int socket, int timeout);

// Função para enviar uma mensagem
void envia_msg(int socket,protocolo *msg);

// Função para receber uma mensagem
protocolo* recebe_msg(int socket,int n_msgs);

// Função para ler uma mensagem
uint8_t ler_msg(protocolo *msg);

// Função para limpar o buffer
void limpa_buffer(char *buffer,int tam);

// Função para imprimir os erros
void d_erro(uint8_t *codigo);

// Função para imprimir uma mensagem
void imprime_msg(protocolo *msg);

//preenche dados com 1s
void padding_dados(uint8_t *dados, int tam);

// Função que envia um pedido de lista
void envia_pedido_lista(int socket);

// Envia uma mensagem com ack
void envia_ack(int socket, uint8_t seq);

// Envia uma mensagem com nack
void envia_nack(int socket, uint8_t seq);

// Envia uma mensagem de erro
void envia_erro(int socket, uint8_t seq);

#endif  // PROTOCOLO_H
