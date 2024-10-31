// Definições e implementações das estruturas e funções de protocolo
#include "protocolo.h"
#include "janela.h"
#include "raw_socket.h"
#include "server.h"

// Função que processa a mensagem do cliente
void processa_mensagem_cliente(int socket, protocolo *msg) 
{
    printf("Processando mensagem do cliente:\n");
    switch (msg->tipo)
    {
        case LISTA:
            printf("Pedido de lista recebido\n\n");
            lista_arquivos(socket);
            break;

        case BAIXAR:
            printf("Pedido de download recebido\n");
            char nome_arquivo[65]; // Buffer para armazenar o nome do arquivo recebido
            memcpy(nome_arquivo, msg->dados, msg->tam); // Copia apenas os dados válidos
            nome_arquivo[msg->tam] = '\0'; // Adiciona um terminador nulo
            printf("Nome do arquivo solicitado: %s\n", nome_arquivo);
            processa_pedido_video(socket, nome_arquivo);
        case ACK:
            printf("ACK recebido\n\n");
            break;

        case NACK:
            printf("NACK recebido\n\n");
            break;

        case FTX:
            printf("FTX recebido\n\n");
            break;
        
        case ERRO:
            printf("ERRO recebido\n\n");
            break;
    }
}


// "/home/diegaum/Redes/code/videos"
// ./videos

void processa_pedido_video(int socket, const char *filename) {
    printf("Recebendo pedido de vídeo\n");

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./videos/%s", filename);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir arquivo");
        envia_erro(socket, 0x01); // Envia erro para o cliente
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Erro ao obter informações do arquivo");
        close(fd);
        envia_erro(socket, 0x01); // Envia erro para o cliente
        return;
    }

    size_t tamanho = st.st_size;
    char descritor[128];
    snprintf(descritor, sizeof(descritor), "tamanho=%zu,data=20230601", tamanho);

    protocolo *descritor_msg = cria_msg(0, DESCRITOR_ARQUIVO, (uint8_t *)descritor, strlen(descritor) + 1);
    envia_msg(socket, descritor_msg);
    exclui_msg(descritor_msg); // Libera descritor_msg após o uso

    uint8_t buffer[PACKET_DATA_MAX_SIZE];
    ssize_t bytes_lidos = 0;

    protocolo *janela[MAX_JANELA];
    memset(janela, 0, MAX_JANELA * sizeof(protocolo*));

    int i = 0;
    for (i = 0; i < MAX_JANELA && (bytes_lidos = read(fd, buffer, sizeof(buffer))) > 0; i++) {
        janela[i] = cria_msg(i, DADOS,  buffer, bytes_lidos);
    }
    int fim_janela = MAX_JANELA-1;
    if (i < MAX_JANELA-1)
        fim_janela = i;

    i = 0; // Índice da janela a enviar
    int inicio_janela = 0;
    int acabou = 0;
    int voltas = 0;
    int ultimo = 0;
    int tentativas = 0;
    int tempo = 0;
    int seq_ultimo = tamanho / PACKET_DATA_MAX_SIZE+1;

    while (acabou == 0) {

        printf("\033[H\033[J");
        printf("Enviando Vídeo:\n");
        printf("Progresso %d%%...\n", (fim_janela*100/seq_ultimo));
        printf("Digite \"exit\" ou \"sair\" para cancelar\n"); 

        fflush(stdout);

        if (espera(STDIN_FILENO, 0) == 0){
            char buffer[1024];
            ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';  // Null-terminate the string
                if (strcmp(buffer, "exit\n") == 0 || strcmp(buffer, "sair\n") == 0) {
                    envia_erro(socket, 0); // Envia erro para o cliente
                    printf("Voltando para o menu...\n");
                    return;
                } else {
                    printf("Você digitou: %s\n", buffer);
                }
            }
        }

        if (i == fim_janela + 1) { //mais 1 para mandar o ultimo
            i = inicio_janela;
            tentativas++;
        }

        if (tentativas == 16) {
            printf("Timeout, sem resposta do cliente após 16 tentativas\n");
            close(fd);
            return;
        }

        ultimo = fim_janela > MAX_JANELA-1 ? fim_janela: MAX_JANELA-1;
        envia_msg(socket, janela[(i+(MAX_JANELA-1))-ultimo]);

        if (tentativas == 9) //se janela deu 10 voltas sem receber, espera 1 seg para receber
            tempo = 1;

        while (espera(socket, tempo) == 0) { // Se chegou um pacote

            tentativas = 0;
            protocolo *resposta = recebe_msg(socket, 1);
            if (resposta == NULL || resposta->inicio != PACKET_START_MARKER) {
                exclui_msg(resposta);
                break;
            }

            if (resposta->tipo == ACK || resposta->tipo == NACK) {
                int sequencia = resposta->tipo == ACK ? resposta->seq: resposta->seq - 1;
                if (sequencia == -1)
                    sequencia = 31;
                if (sequencia % 32 < (inicio_janela-1) % 32)
                    voltas++;

                if (resposta->tipo == ACK && resposta->seq == (seq_ultimo - 1) % 32) {
                    if ((int)((seq_ultimo - 1) / 32) == voltas) {
                        exclui_msg(resposta);
                        acabou = 1;
                        break;
                    }
                }

                int qnt_avancar = sequencia + (voltas * 32);
                for (int j = inicio_janela; j <= qnt_avancar; j++) {
                    if (fim_janela < seq_ultimo - 1) {
                        inicio_janela++;
                        fim_janela++;
                        uint8_t buffer[PACKET_DATA_MAX_SIZE];
                        bytes_lidos = read(fd, buffer, sizeof(buffer));
                        desliza_janela(janela, cria_msg(fim_janela % 32, DADOS, buffer, bytes_lidos));
                    } else {
                        if (inicio_janela < fim_janela) {
                            inicio_janela++;
                        } else if (inicio_janela > fim_janela) {
                            close(fd);
                            return;
                        }
                    }
                }
                if (i < inicio_janela - 1)
                    i = inicio_janela - 1;
            }

            if (resposta->tipo == NACK) {
                i = inicio_janela - 1;
            } else if (resposta->tipo == ERRO) {
                cuidar_erro(resposta);
                return;
            }
            exclui_msg(resposta);
        }
        i++;
    }

    envia_ftx(socket);

    tentativas = 0;
    acabou = 0;
    while (acabou == 0) {
        protocolo *resposta;
        if (tentativas == 16) {
            acabou = 1;
            break;
        }
        if (espera(socket, 1) == 1) {
            envia_ftx(socket);
            tentativas++;
        } else {
            resposta = recebe_msg(socket, 1);
            if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == ACK && resposta->seq == 0) {
                acabou = 1;
                exclui_msg(resposta);
                break;
            }
            tentativas++;
        }
    }
    
    printf("Conexão encerrada\n");
    exclui_janela(janela);
    close(fd); // Certifique-se de fechar o arquivo ao final
}

// Função para listar arquivos de vídeo no diretório
void lista_arquivos(int socket) {
    printf("Tentando abrir o diretório de vídeos\n");
    struct dirent *entry;
    DIR *dp = opendir("./videos");
    if (dp == NULL) {
        envia_erro(socket, 0);
        perror("Erro ao abrir diretório de vídeos\n");
        return;
    }

    // Cria lista de strings com os nomes dos vídeos
    char **lista = NULL;
    int qnt_arquivos = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char *ext = strrchr(entry->d_name, '.'); // Último . no nome do arquivo
            if (ext && (strcmp(ext, ".mp4") == 0 || strcmp(ext, ".avi") == 0)) {
                // Adiciona o nome do arquivo à lista
                lista = realloc(lista, (qnt_arquivos + 1) * sizeof(char *));
                if (lista == NULL) {
                    printf("Erro ao alocar lista de vídeos\n");
                    closedir(dp);
                    return;
                }
                int comprimento = strlen(entry->d_name);
                if (comprimento+1 > 64){
                    printf("Erro nome do video maior que 64");
                    closedir(dp);
                    return;
                }
                lista[qnt_arquivos] = strndup(entry->d_name, comprimento);
                if (lista[qnt_arquivos] == NULL) {
                    printf("Erro ao alocar lista de vídeos\n");
                    closedir(dp);
                    return;
                }
                qnt_arquivos++;
            }
        }
    }
    closedir(dp);

    for (int y = 0; y < qnt_arquivos; y++){
        printf("lista[%d] = %s\n", y, lista[y]);
    }

    if (qnt_arquivos == 0) {
        envia_erro(socket, 0);
        printf("Nenhum arquivo de vídeo encontrado\n");
        return;
    }

    // Confirma que acessou lista e está pronto para enviar
    envia_ack(socket, 0);

    protocolo *janela[MAX_JANELA];
    memset(janela, 0, MAX_JANELA * sizeof(protocolo*));

    // Preenche a janela inicialmente
    for (int i = 0; i < MAX_JANELA && i < qnt_arquivos; i++) {
        janela[i] = cria_msg(i, MOSTRA_TELA, (uint8_t *)lista[i], strlen(lista[i]));
    }

    int i = 0; // Índice da janela a enviar
    int inicio_janela = 0;
    int acabou = 0;
    int voltas = 0;
    int tentativas = 0;
    int tempo = 0;
    // Se tem menos arquivos que o tamanho da janela, já inicia menor
    int fim_janela = qnt_arquivos > MAX_JANELA - 1 ? MAX_JANELA - 1 : qnt_arquivos - 1;
    while (acabou == 0){
        // Se chegou no fim da janela, envia ela novamente
        if (i == fim_janela + 1){ //mais 1 para mandar o ultimo
            i = inicio_janela;
            tentativas++;
        }
        if (tentativas == 16){
            printf("Timeout, sem resposta do cliente apos 16 tentativas\n");
            return;
        }
        
        int ultimo = fim_janela > MAX_JANELA-1 ? fim_janela: MAX_JANELA-1;
        envia_msg(socket, janela[(i+(MAX_JANELA-1))-ultimo]);

        if (tentativas == 9) //se janela deu 10 voltas sem receber, espera 1 seg para receber
            tempo = 1;
        while (espera(socket, tempo) == 0) { // Se chegou um pacote
            tentativas = 0;
            protocolo *resposta = recebe_msg(socket, 1);
            if (resposta == NULL || resposta->inicio != PACKET_START_MARKER) {
                exclui_msg(resposta);
                break;
            }

            if (resposta->tipo == ACK || resposta->tipo == NACK) {
                //seq eh = seq do ack, ou seq - 1 do nack
                int sequencia = resposta->tipo == ACK ? resposta->seq: resposta->seq - 1;
                if (sequencia == -1)
                    sequencia = 31;
                if (sequencia%32 < (inicio_janela-1)%32)
                    voltas++;
                
                // Cliente recebeu o último, acaba
                if (resposta->tipo == ACK && resposta->seq == (qnt_arquivos - 1)%32) {
                    if ((int)((qnt_arquivos - 1)/32) == voltas){
                        exclui_msg(resposta);
                        acabou = 1;
                        break;
                    }
                }

                int qnt_avancar = sequencia + (voltas * 32);
                // Avança pela sequencia recebida pelo ACK ou NACK
                for (int j = inicio_janela; j <= qnt_avancar; j++) {
                    // Se não chegou ao fim
                    if (fim_janela < qnt_arquivos - 1) {
                        inicio_janela++;
                        fim_janela++;
                        uint8_t *dados = (uint8_t *)lista[fim_janela];
                        size_t tam = strlen(lista[fim_janela]);
                        // Anda janela, e adiciona nova msg no final
                        desliza_janela(janela, cria_msg(fim_janela%32, MOSTRA_TELA, dados, tam));
                    } else { // Janela diminui pois chegou no fim
                        if (inicio_janela < fim_janela) {
                            inicio_janela++;
                        } else if(inicio_janela > fim_janela){
                            return;    
                        }
                    }
                }
                if (i < inicio_janela-1)
                    i = inicio_janela-1;
            }
            // Reenvia a janela caso seja NACK
            if (resposta->tipo == NACK){
                i = inicio_janela-1;
            } else if (resposta->tipo == ERRO) {
                cuidar_erro(resposta);
                return;
            }
            exclui_msg(resposta);
        }
        i++;
    }

    // Envia FTX
    envia_ftx(socket);

    tentativas = 0;
    // Espera 1 seg para receber ACK, se não receber manda outro FTX
    acabou = 0;
    while (acabou == 0){
        protocolo *resposta;
        if (tentativas == 16){
            acabou = 1;
            break;
        }
        if (espera(socket, 1) == 1){ //nao recebeu em 1 sec
            envia_ftx(socket);
            tentativas++;
        } else{
            resposta = recebe_msg(socket, 1);
            if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == ACK && resposta->seq == 0){
                acabou = 1;
                exclui_msg(resposta);
                break;
            }
        }
    }

    printf("Conexão encerrada\n");

    exclui_janela(janela);
    exclui_lista(lista, qnt_arquivos);
}

int main() 
{
    int rsocket;
    char interface[] = "eth0"; // Interface de rede (loopback)
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    while (1) {
        printf("Servidor aguardando na interface: %s\n", interface);
        if (espera(rsocket, -1) == 0){// evita busy waiting
            protocolo *msg = recebe_msg(rsocket, 1);
            if (msg && msg->inicio == PACKET_START_MARKER) {
                processa_mensagem_cliente(rsocket, msg);
                exclui_msg(msg);
            } else {
                fprintf(stderr, "Mensagem recebida invalida.\n");
            }
        }
    }

    close(rsocket);
    return 0;
}
