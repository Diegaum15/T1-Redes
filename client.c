#include "define.h"
#include "raw_socket.h"
#include "janela.h"
#include "protocolo.h"
#define SERVER_PORT 12347
#define VIDEO_DIR "/home/diegaum/Redes/code/videos" // Caminho absoluto do diretório

void interface_cliente(int socket, janela_deslizante *janela) 
{
    int escolha;
    char video[256];
    
    while (1) {
        printf("Selecione uma opção:\n");
        printf("1. Listar vídeos disponíveis\n");
        printf("2. Baixar e assistir um vídeo\n");
        printf("3. Sair\n");
        printf("Escolha: ");
        scanf("%d", &escolha);

        switch (escolha) {
            case 1:
                pede_e_recebe_lista(socket);
                break;
            case 2:
                printf("Digite o nome do vídeo a ser baixado: ");
                scanf("%s", video);
                //cliente_manda_baixar(socket, video, janela);
                baixa_arquivo(socket, video, janela);
                break;
            case 3:
                printf("Saindo...\n");
                return;
            default:
                printf("Opção inválida. Tente novamente.\n");
        }
    }
}

//wlp0s20f3

int main() 
{
    int rsocket;
    char interface[] = "lo"; // Interface de rede (loopback)
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);

    janela_deslizante janela;
    inicializa_janela(&janela);

    interface_cliente(rsocket, &janela);

    close(rsocket);
    return 0;
}


/* teste basico para ver a comunicacao do cliente com o servidor
int main() 
{
    int rsocket;
    char interface[] = "lo";
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);
    janela_deslizante janela;
    inicializa_janela(&janela);

    cliente_pede_lista(rsocket, &janela);

    while (1) {
        protocolo *msg = recebe_msg(rsocket, 1);
        if (msg) {
            processa_resposta_servidor(rsocket, msg, &janela);
            exclui_msg(msg);
            if (msg->tipo == FTX) {
                break;
            }
        } else {
            fprintf(stderr, "Erro ao receber mensagem do servidor.\n");
        }
    }

    close(rsocket);
    return 0;
}
*/


/*
int main() {
    int rsocket;
    char interface[] = "lo";
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);

    janela_deslizante janela;
    inicializa_janela(&janela);
    printf("Janela deslizante inicializada.\n");

    cliente_pede_lista(rsocket, &janela);

    protocolo *msg_recebida;
    while ((msg_recebida = recebe_msg(rsocket, 1)) != NULL) 
    {
        printf("Mensagem recebida do servidor no programa client.c:\n");
        imprime_msg(msg_recebida);

        printf("Tipo da mensagem no programa client.c : %d\n", msg_recebida->tipo);
        switch (msg_recebida->tipo) 
        {
            case ACK:
                printf("ACK recebido\n");
                break;
            case NACK:
                printf("NACK recebido\n");
                break;
            case MOSTRA_TELA:
                printf("Dados recebidos: %s\n", msg_recebida->dados);
                envia_ack(rsocket, msg_recebida->seq);
                break;
            case FTX:
                printf("FTX recebido\n");
                envia_ack(rsocket, msg_recebida->seq);
                close(rsocket);
                return 0;
            case ERRO:
                printf("ERRO recebido\n");
                break;
            case LISTA:
                printf("Pedido de lista recebido\n");
            //default:
             //   printf("Tipo de mensagem não reconhecido: %d\n", msg_recebida->tipo);
               // envia_erro(rsocket, msg_recebida->seq);
                break;
        }

        exclui_msg(msg_recebida);
    }

    close(rsocket);
    return 0;
}
*/

/* teste do envio dos varios tipos de mensagem
void envia_mensagem_teste(int socket, uint8_t tipo, uint8_t *dados, int tamanho) {
    protocolo *msg = cria_msg(1, tipo, dados, tamanho);
    if (!msg) {
        fprintf(stderr, "Erro ao criar mensagem para envio.\n");
        return;
    }
    envia_msg(socket, msg);
    printf("Mensagem do tipo %d enviada para o servidor.\n", tipo);
    exclui_msg(msg);
}


int main() {
    int rsocket;
    char interface[] = "lo";
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);

    // Enviar mensagem de teste do tipo DADOS
    uint8_t dados_dados[] = "Mensagem de dados";
    envia_mensagem_teste(rsocket, DADOS, dados_dados, strlen((char *)dados_dados));

    // Enviar mensagem de teste do tipo ACK
    uint8_t dados_ack[] = "ACK";
    envia_mensagem_teste(rsocket, ACK, dados_ack, strlen((char *)dados_ack));

    // Enviar mensagem de teste do tipo NACK
    uint8_t dados_nack[] = "NACK";
    envia_mensagem_teste(rsocket, NACK, dados_nack, strlen((char *)dados_nack));

    // Enviar mensagem de teste do tipo FTX
    uint8_t dados_ftx[] = "FTX";
    envia_mensagem_teste(rsocket, FTX, dados_ftx, strlen((char *)dados_ftx));

    // Enviar mensagem de teste do tipo ERRO
    uint8_t dados_erro[] = "ERRO";
    envia_mensagem_teste(rsocket, ERRO, dados_erro, strlen((char *)dados_erro));

    close(rsocket);

    return 0;
}

*/