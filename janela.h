#ifndef JANELA_H
#define JANELA_H

// Inicializa a estrutura da janela deslizante
void inicializa_janela(janela_deslizante *janela);

// Envia uma mensagem com janela deslizante
void envia_mensagem_com_janela(int socket, protocolo *msg, janela_deslizante *janela);

// Desliza a janela
void slide_janela(janela_deslizante *janela);

// Processa confirmações do servidor
void processa_confirmacao_janela(int socket, janela_deslizante *janela);

// Envia uma mensagem com ack
void envia_ack(int socket, uint8_t seq);

// Envia uma mensagem com nack
void envia_nack(int socket, uint8_t seq);

// Envia uma mensagem de erro
void envia_erro(int socket, uint8_t seq);

// Função que recebe a lista de videos disponíveis enviada pelo servidor
void recebe_lista(int socket);

// Função que baixa um arquivo do servidor
void baixa_arquivo(int socket, const char *filename, janela_deslizante *janela);

// Funcao que abre um video
void abre_video(const char *filename);

// // Função que processa a resposta do servidor
void processa_resposta_servidor(int socket, protocolo *msg, janela_deslizante *janela);

// Função que responde ao servidor com ACK ou NACK
void responde_servidor(int socket, protocolo *msg);

// Função que envia uma mensagem de pedido de lista de arquivos
void cliente_manda_lista(int socket, janela_deslizante *janela);

// Função que envia uma mensagem de pedido de download de um arquivo
void cliente_manda_baixar(int socket, const char *video, janela_deslizante *janela);

// Funcao para listar arquivos de vídeo no diretório
void lista_arquivos(int socket);

// Função para enviar um arquivo ao cliente
void envia_arquivo(int socket, const char *filename);

// Função para mandar os arquivos em partes
void envia_partes_arquivo(int socket, uint8_t seq_inicial, const char *arquivo);

// Função que processa a mensagem do cliente
void processa_mensagem_cliente(int socket, protocolo *msg);

#endif  // JANELA_H
