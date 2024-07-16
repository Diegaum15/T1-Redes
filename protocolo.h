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
//se timeout for -1, espera indeterminadamente, se for 0 so checa socket(utilizado para polling)
int espera(int socket, int timeout);

// Função para enviar uma mensagem
void envia_msg(int socket,protocolo *msg);

// Função para receber uma mensagem
protocolo* recebe_msg(int socket,int n_msgs);

// Função para ler uma mensagem
uint8_t ler_msg(protocolo *msg);

// zera os bytes de um buffer
void limpa_buffer(char *buffer,int tam);

// Função para imprimir uma mensagem
void imprime_msg(protocolo *msg);

// preenche o resto de dados(de tam até 64) com 1s
void padding_dados(uint8_t *dados, size_t tam);

// Função que envia um pedido de lista
void envia_pedido_lista(int socket);

// Função que envia uma mensagem de ACK
void envia_ack(int socket, uint8_t seq);

// Função que envia uma mensagem NACK 
void envia_nack(int socket, uint8_t seq);

// Função que envia uma mensagem FTX
void envia_ftx(int socket);

// Função que envia uma mensagem de erro
void envia_erro(int socket, uint8_t seq);

#endif  // PROTOCOLO_H
