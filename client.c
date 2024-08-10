#include "raw_socket.h"
#include "janela.h"
#include "client.h"

void pede_e_recebe_lista(int socket) {
    int tentativas = 0;
    while (tentativas < 16) {
        envia_pedido_lista(socket);

        if (espera(socket, 2) == 1) {
            printf("Timeout pedindo lista - tentando novamente - %d\n", tentativas + 1);
            tentativas++;
        } else {
            protocolo *resposta = recebe_msg(socket, 1);
            if (resposta != NULL && resposta->inicio == PACKET_START_MARKER) {
                if (resposta->tipo == ACK || resposta->tipo == MOSTRA_TELA) {

                    if (resposta->tipo == ACK) {
                        exclui_msg(resposta);
                        espera(socket, -1);
                        resposta = recebe_msg(socket, 1);
                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == MOSTRA_TELA) {
                        } else {
                            exclui_msg(resposta);
                            return;
                        }
                    } 
                    tentativas = 0;
                    protocolo *janela[MAX_JANELA];
                    memset(janela, 0, MAX_JANELA * sizeof(protocolo*));
                    char **lista = NULL;
                    int seq_esperado = 0;
                    // enquanto nao acabar a transmissao
                    while (resposta == NULL || resposta->tipo != FTX) {
                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && (resposta->tipo == MOSTRA_TELA || resposta->tipo == FTX)){
                            if (resposta->tipo == ERRO){
                                cuidar_erro(resposta);
                                return;
                            }

                            if (resposta->seq == (seq_esperado%32) && resposta->tipo == MOSTRA_TELA) {
                                janela[0] = resposta; // 0 eh a resposta
                                int i = 1; // quantidade de pacotes a processar
                                while (janela[i] != NULL && i < MAX_JANELA) {                                        
                                    i++;
                                }
                                // guarda o(s) nome(s) recebido(s) em lista
                                lista = realloc(lista, (seq_esperado + i) * sizeof(char*)); // aumenta a lista
                                if (lista == NULL) {
                                    printf("Erro ao alocar lista de videos");
                                    exclui_janela(janela);
                                    return;
                                }
                                for (int j = 0; j < i; j++) {
                                    lista[seq_esperado + j] = strndup((char*)janela[0]->dados, janela[0]->tam);
                                    if (lista[seq_esperado + j] == NULL) {
                                        printf("Erro ao alocar lista de videos");
                                        exclui_lista(lista, seq_esperado + j);
                                        exclui_janela(janela);
                                        return;
                                    }
                                    //libera pacotes processados da janela
                                    desliza_janela(janela, NULL);
                                }

                                //manda ack e adiciona sequencia
                                envia_ack(socket, (seq_esperado+i-1)%32);
                                //limite de seq eh 32
                                seq_esperado += i;
                            } else { //nao eh o esperado
                                envia_nack(socket, seq_esperado%32);
                                int diferenca = resposta->seq - seq_esperado%32;
                                if (diferenca > 0 && diferenca < MAX_JANELA)
                                    janela[diferenca] = resposta;
                            }
                        }
                        

                        
                        if (espera(socket, 7) == 0){
                            resposta = recebe_msg(socket, 1);
                        }
                        else{
                            printf("timeout, sem resposta do servidor a 7 segundos\n");
                            exclui_lista(lista, seq_esperado);
                            exclui_janela(janela);
                            return;
                        }
                    }
                    envia_ack(socket, 0);

                    tentativas = 0;
                    int acabou = 0;
                    //espera 2 sec, se receber FTX denovo, retorna ack (limite de 16 retornos)
                    while (acabou == 0){
                        protocolo *resposta;
                        if (tentativas == 16){
                            acabou = 1;
                            break;
                        }
                        if (espera(socket, 2) == 0){ //se recebeu ftx denovo
                            resposta = recebe_msg(socket, 1);
                            if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == FTX){
                                envia_ack(socket, 0);
                                tentativas++;
                            }
                            exclui_msg(resposta);
                        } else{
                            acabou = 1;
                            break;
                        }
                    }

                    for(int y = 0 ; y < seq_esperado; y++){
                        printf("lista[%d] = %s\n",y, lista[y]);
                    }

                    exclui_janela(janela);
                    exclui_lista(lista, seq_esperado);
                    printf("conexão encerrada\n");
                    return;
                } else {
                    if (resposta->tipo == NACK) {
                        tentativas++;
                    }
                    if (resposta->tipo == ERRO) {
                        tentativas++;
                    }
                }
            } 
        }
    }
    printf("Conexão falhou depois de %d tentativas (timeout)\n", tentativas);
    return;
}

void pede_e_recebe_video(int socket, const char *nome_video) {
    int tentativas = 0;
    int seq_esperado = 0;
    size_t tamanho = 0;
    protocolo *resposta = NULL;
    protocolo *janela[MAX_JANELA];
    memset(janela, 0, MAX_JANELA * sizeof(protocolo*));

    // Envia pedido de vídeo
    envia_pedido_video(socket, nome_video); // Função adaptada para enviar tipo "BAIXAR"

    while (tentativas < 16) {
        if (espera(socket, 2) == 1) {
            printf("Timeout pedindo vídeo - tentando novamente - %d\n", tentativas + 1);
            tentativas++;
            envia_pedido_video(socket, nome_video); // Função adaptada para enviar tipo "BAIXAR"
        } else {
            resposta = recebe_msg(socket, 1);
            if (resposta != NULL && resposta->inicio == PACKET_START_MARKER) {
                if (resposta->tipo == ACK || resposta->tipo == DESCRITOR_ARQUIVO) {
                    FILE *video_file = fopen(nome_video, "wb");
                    if (!video_file) {
                        printf("Erro ao abrir arquivo de vídeo para escrita\n");
                        return;
                    }
                    if (resposta->tipo == ACK) {
                        exclui_msg(resposta);
                        espera(socket, -1);
                        resposta = recebe_msg(socket, 1);
                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == DESCRITOR_ARQUIVO) {
                            sscanf((char*)resposta->dados, "tamanho=%ld", &tamanho);
                            tamanho = tamanho / PACKET_DATA_MAX_SIZE+1;
                        } else {
                            exclui_msg(resposta);
                            fclose(video_file);
                            return;
                        }
                    }
                    else{
                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == DESCRITOR_ARQUIVO) {
                            sscanf((char*)resposta->dados, "tamanho=%ld", &tamanho);
                            tamanho = tamanho / PACKET_DATA_MAX_SIZE+1;
                        }
                    }

                    tentativas = 0;

                    while (resposta == NULL || resposta->tipo != FTX) 
                    {

                        printf("\033[H\033[J");
                        printf("Baixando Vídeo:\n");
                        printf("Progresso %ld%%...\n", (seq_esperado*100/tamanho));
                        printf("Digite \"exit\" ou \"sair\" para cancelar\n"); 

                        fflush(stdout);

                        if (espera(STDIN_FILENO, 0) == 0){
                            char buffer[1024];
                            ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));
                            if (bytesRead > 0) {
                                buffer[bytesRead] = '\0';  // Null-terminate the string
                                if (strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "sair\n") == 0) {
                                    envia_erro(socket, 0); // Envia erro para o server
                                    printf("Voltando para o menu...\n");
                                    return;
                                } else {
                                    printf("Você digitou: %s\n", buffer);
                                }
                            }
                        }

                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && (resposta->tipo == DADOS || resposta->tipo == FTX)) {

                            if (resposta->seq == (seq_esperado % 32) && resposta->tipo == DADOS) {
                                janela[0] = resposta; // 0 é a resposta
                                int i = 1;
                                while (janela[i] != NULL && i < MAX_JANELA) {
                                    i++;
                                }

                                for (int j = 0; j < i; j++) {
                                    fwrite(janela[j]->dados, 1, janela[j]->tam, video_file);
                                    exclui_msg(janela[j]);
                                    janela[j] = NULL;
                                }

                                desliza_janela(janela, NULL);

                                envia_ack(socket, (seq_esperado + i - 1) % 32);
                                seq_esperado += i;
                            } else {
                                envia_nack(socket, seq_esperado % 32);
                                int diferenca = resposta->seq - seq_esperado % 32;
                                if (diferenca > 0 && diferenca < MAX_JANELA)
                                    janela[diferenca] = resposta;
                            }
                        } else {
                            if (resposta != NULL) {
                                if (resposta->tipo == ERRO){
                                    cuidar_erro(resposta);
                                    return;
                                }
                                exclui_msg(resposta);
                            }
                        }
                        resposta = recebe_msg(socket, 1);
                    }

                    envia_ack(socket, 0);

                    tentativas = 0;
                    int acabou = 0;
                    //espera 2 sec, se receber FTX denovo, retorna ack (limite de 16 retornos)
                    while (acabou == 0){
                        protocolo *resposta;
                        if (tentativas == 16){
                            acabou = 1;
                            break;
                        }
                        if (espera(socket, 2) == 0){ //se recebeu ftx denovo
                            resposta = recebe_msg(socket, 1);
                            if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == FTX){
                                envia_ack(socket, 0);
                                tentativas++;
                            }
                            exclui_msg(resposta);
                        } else{
                            acabou = 1;
                            break;
                        }
                    }

                    abre_video(nome_video);
                    exclui_janela(janela);
                    printf("conexão encerrada\n");
                    fclose(video_file);
                    return;
                }
                else {
                    if (resposta->tipo == NACK) {
                        tentativas++;
                    }
                    if (resposta->tipo == ERRO) {
                        tentativas++;
                    }
                }
            } 
        }
    }
    printf("Conexão falhou depois de %d tentativas (timeout)\n", tentativas);
    return;
}


void interface_cliente(int socket) 
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
                pede_e_recebe_video(socket, video);
                break;
            case 3:
                printf("Saindo...\n");
                return;
            default:
                printf("Opção inválida. Tente novamente.\n");
        }
    }
}

int main(int argc, char *argv[]) {

    int rsocket;
    char interface[] = "eth0"; // Interface de rede (loopback)
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);

    interface_cliente(rsocket);

    close(rsocket);
    return 0;
}