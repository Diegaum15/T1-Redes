#include "define.h"
#include "janela.h"
#include <linux/if_packet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#define VIDEO_DIR "/home/diegaum/Redes/code/videos" // Caminho absoluto do diretório

// Caso que o cliente manda uma mensagem do tipo lista "01010"
// Servidor pode responder com ACK , NACK, MOSTRA_TELA ou ERRO
// Cliente responde com ACK ou NACK ate o servidor mande FTX(fim transmissao) e o cliente responda com ACK ou NACK

// Caso que o cliente manda uma mensagem do tipo baixar "01011" e o servidor responde com ACK , NACK , ERRO ou DESCRITROR_ARQUIVO
// Cliente responde com ACK, NACK ou ERRO ate o servidor mandar os DADOS
// Cliente responde com ACK ou NACK ate o servidor mandar FTX(fim transmissao) e o cliente responda com ACK ou NACK

// Inicializa a estrutura da janela deslizante
void inicializa_janela(janela_deslizante *janela) 
{
    // Inicializa a base da janela como 0 (primeiro índice do vetor)
    janela->base = 0;

    // Inicializa o índice da próxima mensagem a ser enviada como 0
    janela->next_seq = 0;

    // Inicializa todos os elementos do vetor de mensagens como NULL
    for (int i = 0; i < MAX_JANELA; i++) 
    {
        janela->buffer[i] = NULL;
    }
}

//Esta função adiciona a mensagem no buffer da janela e a envia pelo socket. Em seguida, incrementa o índice da próxima sequência.
// Função que envia uma mensagem e a adiciona na janela deslizante
void envia_mensagem_com_janela(int socket, protocolo *msg, janela_deslizante *janela) 
{
    // Adiciona a mensagem na próxima posição da janela
    janela->buffer[janela->next_seq % MAX_JANELA] = msg;

    // Envia a mensagem
    envia_msg(socket, msg);
    printf("\n");
    printf("Mensagem enviada dentro da funcao envia_mensagem_com_janela.\n");
    printf("Tipo da mensagem: %d\n", msg->tipo);
    printf("Sequência da mensagem: %d\n", msg->seq);
    printf("Marcador de inicio : %d\n", msg->inicio);
    printf("\n");

    // Atualiza o índice da próxima mensagem a ser enviada
    janela->next_seq++;
}

// Função que desliza a janela deslizante
void slide_janela(janela_deslizante *janela) 
{
    // Incrementa a base da janela
    janela->base++;

    // Libera a memória da mensagem na base da janela
    exclui_msg(janela->buffer[janela->base % MAX_JANELA]);

    // Atualiza a mensagem na base da janela para NULL
    janela->buffer[janela->base % MAX_JANELA] = NULL;
}

//  Recebe confirmações (ACK ou NACK) do servidor e processa de acordo, deslizando a janela conforme necessário
void processa_confirmacao_janela(int socket, janela_deslizante *janela) 
{
    protocolo *msg_recebida = recebe_msg(socket, 1);
    if (!msg_recebida) {
        fprintf(stderr, "Erro ao receber mensagem de confirmação.\n");
        return;
    }
    uint8_t tipo_msg = ler_msg(msg_recebida);
    if (tipo_msg == ACK) {
        uint8_t ack = msg_recebida->dados[0];
        while (janela->base <= ack) {
            slide_janela(janela);
        }
    } else if (tipo_msg == NACK) {
        uint8_t nack = msg_recebida->dados[0];
        envia_msg(socket, janela->buffer[nack % MAX_JANELA]);
    }
    exclui_msg(msg_recebida);
}

// Função que envia uma mensagem de ACK
void envia_ack(int socket, uint8_t seq) 
{
    uint8_t dados[1] = { seq };
    protocolo *ack_msg = cria_msg(seq, ACK, dados, sizeof(dados));
    envia_msg(socket, ack_msg);
    exclui_msg(ack_msg);
}

// Função que envia uma mensagem NACK 
void envia_nack(int socket, uint8_t seq) 
{
    uint8_t dados[1] = { seq };
    protocolo *nack_msg = cria_msg(seq, NACK, dados, sizeof(dados));
    envia_msg(socket, nack_msg);
    exclui_msg(nack_msg);
}

// Função que envia uma mensagem de erro
void envia_erro(int socket, uint8_t seq) 
{
    uint8_t dados[1] = { seq };
    protocolo *erro_msg = cria_msg(seq, ERRO, dados, sizeof(dados));
    envia_msg(socket, erro_msg);
    exclui_msg(erro_msg);
}

// Função que recebe a lista de videos disponíveis enviada pelo servidor
void recebe_lista(int socket) 
{
    protocolo *resposta;
    resposta = recebe_msg(socket, 1);
    printf("Dentro da funcao recebe_lista\n");
    printf("Tipo da mensagem recebida: %d\n", resposta->tipo);
    printf("\n");
    if (resposta) 
    {
        printf("Tipo da mensagem recebida: %d\n", resposta->tipo);
        if (resposta->tipo == MOSTRA_TELA) {
            printf("Lista de vídeos disponíveis:\n%s\n", resposta->dados);
        } else {
            printf("Erro: Tipo de mensagem inesperado\n");
        }
        exclui_msg(resposta);
    } else {
        printf("Erro ao receber lista de vídeos\n");
    }
}

// Funcao que abre um video
void abre_video(const char *filename) 
{
    char comando[256];
    snprintf(comando, sizeof(comando), "xdg-open %s", filename);
    system(comando);
}

// Cria uma mensagem para solicitar o download de um arquivo. Em seguida, abre o arquivo para escrita e entra em um loop para receber as partes do arquivo até receber uma mensagem FTX (fim de transmissão). Envia ACK para cada parte recebida corretamente.
// Função que baixa um arquivo do servidor
void baixa_arquivo(int socket, const char *filename, janela_deslizante *janela) 
{
    protocolo *msg = cria_msg(janela->next_seq, BAIXAR, (uint8_t *)filename, strlen(filename));
    envia_mensagem_com_janela(socket, msg, janela);
    exclui_msg(msg);

    FILE *arquivo = fopen(filename, "wb");
    if (!arquivo) {
        perror("Erro ao abrir arquivo para escrita");
        return;
    }

    protocolo *resposta;
    int ftx_recebido = 0;

    while (!ftx_recebido) {
        resposta = recebe_msg(socket, 1);
        if (resposta) {
            switch (resposta->tipo) {
                case DADOS:
                    fwrite(resposta->dados, 1, resposta->tam, arquivo);
                    envia_ack(socket, resposta->seq);
                    break;
                case FTX:
                    ftx_recebido = 1;
                    envia_ack(socket, resposta->seq);
                    break;
                default:
                    envia_erro(socket, resposta->seq);
                    break;
            }
            exclui_msg(resposta);
        }
    }

    fclose(arquivo);
    printf("Arquivo %s baixado com sucesso.\n", filename);

    // Chama o reprodutor de vídeo após o download do arquivo
    abre_video(filename);
}

// Processa diferentes tipos de mensagens recebidas do servidor e executa as ações apropriadas, como enviar ACK/NACK, atualizar a janela deslizante ou exibir dados
// Função que processa a resposta do servidor
void processa_resposta_servidor(int socket, protocolo *msg, janela_deslizante *janela)
{
    printf("Dentro da funcao processa_resposta_servidor\n");
    printf("Tipo da mensagem recebida: %d\n", msg->tipo);
    printf("\n");

    switch (msg->tipo) 
    {
        case ACK:
            printf("ACK recebido\n");
            printf("\n");
            janela->base++;
            if (janela->base == janela->next_seq) {
                // Janela está vazia
                inicializa_janela(janela);
            }
            break;
        case NACK:
            printf("NACK recebido\n");
            printf("\n");
            // Reenviar mensagens a partir da base
            for (int i = janela->base; i < janela->next_seq; i++) {
                envia_msg(socket, janela->buffer[i % MAX_JANELA]);
            }
            break;
        case MOSTRA_TELA:
            printf("MOSTRA_TELA recebido\n");
            printf("Dados recebidos: %s\n", msg->dados);
            printf("\n");
            //envia_ack(socket, msg->seq);
            break;
        case FTX:
            printf("FTX recebido\n");
            printf("\n");    
            //envia_ack(socket, msg->seq);
            break;
        case DADOS:
            printf("Dados recebidos: %s\n", msg->dados);
            printf("\n");
            //envia_ack(socket, msg->seq);
            break;
        case DESCRITOR_ARQUIVO:
            printf("Descritor de arquivo recebido\n");
            printf("\n");
            //envia_ack(socket, msg->seq);
            break;
        //default:
           // printf("Tipo de mensagem não reconhecido: %d\n", msg->tipo);
           // envia_erro(socket, msg->seq);
            //break;
    }
}

// Função que responde ao servidor com ACK ou NACK
void responde_servidor(int socket, protocolo *msg) 
{
    uint8_t tipo_msg = ler_msg(msg);

    if (tipo_msg != ERRO) {
        envia_ack(socket, msg->seq);
    } else {
        envia_nack(socket, msg->seq);
    }
}

// Função que envia uma mensagem de pedido de lista de arquivos
void cliente_manda_lista(int socket, janela_deslizante *janela)
{
    protocolo *msg = cria_msg(janela->next_seq, LISTA, NULL, 0);
    envia_mensagem_com_janela(socket, msg, janela);
    printf("Mensagem do tipo 'lista' enviada para o servidor.\n");
    printf("\n");
}

// Função que envia uma mensagem de pedido de download de um arquivo
void cliente_manda_baixar(int socket, const char *video, janela_deslizante *janela) 
{
    protocolo *msg = cria_msg(1, BAIXAR, (uint8_t *)video, strlen(video));
    if (!msg) {
        fprintf(stderr, "Erro ao criar mensagem para envio.\n");
        return;
    }
    envia_mensagem_com_janela(socket, msg, janela);
    printf("Mensagem do tipo 'baixar' enviada para o servidor.\n");
    exclui_msg(msg);
}

/*
Verificações de Erro: Aumentar a quantidade de verificações de erro, especialmente na alocação de memória e nas operações de I/O.
Logs: Adicionar mais logs detalhados para facilitar o debugging e o monitoramento.
Modularização: Separar as funções em diferentes arquivos, como protocolo.c, janela.c, etc., para facilitar a manutenção e a leitura do código.
Documentação: Adicionar comentários e documentação para explicar o propósito e o funcionamento de cada função, parâmetro e estrutura.
*/

// Funcoes para o servidor //
//                         // 
//                         //
// Caso que o cliente manda uma mensagem do tipo lista "01010"
// Servidor pode responder com ACK , NACK, MOSTRA_TELA ou ERRO
// Cliente responde com ACK ou NACK ate o servidor mande FTX(fim transmissao) e o cliente responda com ACK ou NACK

// Caso que o cliente manda uma mensagem do tipo baixar "01011" e o servidor responde com ACK , NACK , ERRO ou DESCRITROR_ARQUIVO
// Cliente responde com ACK, NACK ou ERRO ate o servidor mandar os DADOS
// Cliente responde com ACK ou NACK ate o servidor

// Funcao para listar arquivos de vídeo no diretório
void lista_arquivos(int socket) 
{
    printf("Tentando abrir o diretório de vídeos: %s\n", VIDEO_DIR);
    struct dirent *entry;
    DIR *dp = opendir(VIDEO_DIR);
    if (dp == NULL) {
        perror("Erro ao abrir diretório de vídeos");
        return;
    }

    char lista[PACKET_DATA_MAX_SIZE * 10] = {0}; // Para acomodar múltiplos nomes de arquivos

    while ((entry = readdir(dp))) {
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && (strcmp(ext, ".mp4") == 0 || strcmp(ext, ".avi") == 0)) {
                strncat(lista, entry->d_name, sizeof(lista) - strlen(lista) - 2);
                strncat(lista, ",", sizeof(lista) - strlen(lista) - 1);
            }
        }
    }

    closedir(dp);

    // Remove última vírgula
    size_t len = strlen(lista);
    if (len > 0 && lista[len - 1] == ',') {
        lista[len - 1] = '\0';
    }

    // Enviar a lista de arquivos
    protocolo *lista_msg = cria_msg(0, MOSTRA_TELA, (uint8_t *)lista, strlen(lista));
    printf("Lista de arquivos disponíveis: %s\n", lista);
    envia_msg(socket, lista_msg);
    imprime_msg(lista_msg);
    printf("\n");
    free(lista_msg);
}

// Função para enviar um arquivo ao cliente
void envia_arquivo(int socket, const char *filename) 
{
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/%s", VIDEO_DIR, filename);
    printf("Tentando abrir o arquivo: %s\n", filepath);

    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("Erro ao abrir arquivo para leitura");
        envia_erro(socket, 0x01); // Acesso negado
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Erro ao obter informações do arquivo");
        close(fd);
        envia_erro(socket, 0x01);
        return;
    }

    size_t tamanho = st.st_size;
    char descritor[128];
    snprintf(descritor, sizeof(descritor), "tamanho=%zu,data=20230601", tamanho);

    protocolo *descritor_msg = cria_msg(0, DESCRITOR_ARQUIVO, (uint8_t *)descritor, strlen(descritor));
    envia_msg(socket, descritor_msg);
    free(descritor_msg);

    uint8_t buffer[PACKET_DATA_MAX_SIZE];
    ssize_t bytes_lidos;

    while ((bytes_lidos = read(fd, buffer, sizeof(buffer))) > 0) {
        protocolo *dados_msg = cria_msg(0, DADOS, buffer, bytes_lidos);
        envia_msg(socket, dados_msg);
        free(dados_msg);
    }

    close(fd);

    protocolo *ftx_msg = cria_msg(0, FTX, NULL, 0);
    envia_msg(socket, ftx_msg);
    free(ftx_msg);
}

// Função para mandar os arquivos em partes
void envia_partes_arquivo(int socket, uint8_t seq_inicial, const char *arquivo) 
{
    FILE *fp = fopen(arquivo, "rb");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir o arquivo.\n");
        envia_erro(socket, seq_inicial);
        return;
    }

    uint8_t buffer[63]; // Supondo que o tamanho máximo de dados por mensagem é 63 bytes
    size_t bytes_lidos;
    uint8_t seq = seq_inicial;
    int base = seq_inicial;
    int n_msgs_enviadas = 0;

    while ((bytes_lidos = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        protocolo *dados_msg = cria_msg(seq, DADOS, buffer, bytes_lidos);
        envia_msg(socket, dados_msg);
        exclui_msg(dados_msg);
        seq++;
        n_msgs_enviadas++;

        // Controle de fluxo com janela deslizante
        if (n_msgs_enviadas >= 5) {
            // Espera pelo ACK das mensagens enviadas antes de continuar
            for (int i = 0; i < 5; i++) {
                protocolo *ack_msg = recebe_msg(socket, 1);
                if (ack_msg && ack_msg->tipo == ACK && ack_msg->seq == base) {
                    base++;
                    n_msgs_enviadas--;
                    exclui_msg(ack_msg);
                } else {
                    // Se não receber o ACK esperado, reenvia as mensagens a partir da base
                    fseek(fp, (base - seq_inicial) * sizeof(buffer), SEEK_SET);
                    seq = base;
                    n_msgs_enviadas = 0;
                    break;
                }
            }
        }
    }

    fclose(fp);

    // Envia FTX para indicar o fim da transmissão
    protocolo *ftx_msg = cria_msg(seq, FTX, NULL, 0);
    envia_msg(socket, ftx_msg);
    exclui_msg(ftx_msg);
}

// Função que processa a mensagem do cliente
void processa_mensagem_cliente(int socket, protocolo *msg) 
{
    printf("Processando mensagem do cliente:\n");
    imprime_msg(msg); // Supondo que imprime_msg imprime detalhes da mensagem
    int marcador_inicio = 126;

    switch (msg->tipo) 
    {
        case LISTA:
            printf("Pedido de lista recebido\n");

            // Verifica se a mensagem é válida e envia a resposta correspondente
            if (marcador_inicio == msg->inicio) 
            {
                printf("Mensagem válida\n");
                //envia_ack(socket, msg->seq);
                lista_arquivos(socket);

            } else {
                // Se a mensagem for inválida, envie NACK
                envia_nack(socket, msg->seq);
            }
            break;

        case BAIXAR:
            printf("Pedido de download recebido\n");
            //envia_ack(socket, msg->seq);
            envia_partes_arquivo(socket, msg->seq + 1, (char *)msg->dados);
            break;

        case ACK:
            printf("ACK recebido\n");
            // Processa ACK conforme necessário
            break;

        case NACK:
            printf("NACK recebido\n");
            // Processa NACK conforme necessário
            break;

        case FTX:
            printf("FTX recebido\n");
            // Processa FTX conforme necessário
            break;

        //default:
          //  printf("Tipo de mensagem não reconhecido: %d\n", msg->tipo);
            //envia_erro(socket, msg->seq);
            //break;
    }
}