#ifndef JANELA_H
#define JANELA_H

// Inicializa a estrutura da janela deslizante
void inicializa_janela(janela_deslizante *janela);

// Desliza uma janela, passa quem sera o ultimo da janela
void desliza_janela(protocolo *janela[MAX_JANELA], protocolo *next);

// Função que envia pedido e recebe lista de arquivos disponíveis no servidor
void pede_e_recebe_lista(int socket);

// Função que baixa um arquivo do servidor
void baixa_arquivo(int socket, const char *filename, janela_deslizante *janela);

// Funcao que abre um video
void abre_video(const char *filename);

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

// Funcao que lida com o possiveis erros
void cuidar_erro(protocolo *msg);

// Aloca lista de strings
char **cria_lista(int tamanho);

// Libera lista de strings
void exclui_lista(char **lista, int tamanho);

#endif  // JANELA_H
