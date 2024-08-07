#include "janela.h"
#include <signal.h>
#include <setjmp.h>
#define VIDEO_DIR "/home/diegaum/Redes/code/videos" // Caminho absoluto do diretório // Diretório onde os vídeos estão armazenados
static jmp_buf env;

volatile sig_atomic_t interrompido;


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

void desliza_janela(protocolo *janela[MAX_JANELA], protocolo *next)
{
    exclui_msg(janela[0]);
    for (int i = 0; i < MAX_JANELA-1; i++){
        janela[i] = janela[i+1];
    }
    janela[MAX_JANELA-1] = next;
}

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
                            if (resposta->tipo == ERRO)
                                cuidar_erro(resposta);

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
                
                    exclui_janela(janela);
                    exclui_lista(lista, seq_esperado);
                    printf("conexão encerrada\n");
                    return;
                } else {
                    if (resposta->tipo == NACK) {
                        printf("recebeu NACK\n");
                        tentativas++;
                    }
                    if (resposta->tipo == ERRO) {
                        printf("recebeu ERRO\n");
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
    protocolo *resposta = NULL;
    protocolo *janela[MAX_JANELA];
    memset(janela, 0, MAX_JANELA * sizeof(protocolo*));
    FILE *video_file = fopen("received_video.mp4", "wb"); // Salva o vídeo com um nome fixo para depuração
    if (!video_file) {
        printf("Erro ao abrir arquivo de vídeo para escrita\n");
        return;
    }

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
                    if (resposta->tipo == ACK) {
                        exclui_msg(resposta);
                        espera(socket, -1);
                        resposta = recebe_msg(socket, 1);
                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == DESCRITOR_ARQUIVO) {
                        } else {
                            exclui_msg(resposta);
                            fclose(video_file);
                            return;
                        }
                    } 

                    tentativas = 0;

                    while (resposta == NULL || resposta->tipo != FTX) 
                    {
                        if (sigsetjmp(env, 1) != 0) {
                            printf("Interrupção durante a recepção do vídeo. Voltando ao menu principal.\n");
                            fclose(video_file);
                            return;
                        }

                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && (resposta->tipo == DADOS || resposta->tipo == FTX)) {
                            if (resposta->tipo == ERRO)
                                cuidar_erro(resposta);

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
                                exclui_msg(resposta);
                            }
                        }
                        resposta = recebe_msg(socket, 1);
                    }

                    exclui_janela(janela);
                    fclose(video_file);
                    return;
                }
            } 
        }
    }

    fclose(video_file);
}

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
    int seq_ultimo = tamanho / PACKET_DATA_MAX_SIZE+1;

    while (acabou == 0) {
        if (interrompido) { // Verifica se houve interrupção
            printf("Interrupção detectada. Encerrando envio...\n");
            close(fd);
            envia_erro(socket, 0x02); // Opcional: Envia erro para o cliente informando a interrupção
            return;
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

        int tempo = 0;
        if (tentativas == 9) //se janela deu 10 voltas sem receber, espera 1 seg para receber
            tempo = 1;

        while (espera(socket, tempo) == 0) { // Se chegou um pacote
            if (interrompido) { // Verifica se houve interrupção
                printf("Interrupção detectada. Encerrando envio...\n");
                close(fd);
                envia_erro(socket, 0x02); // Opcional: Envia erro para o cliente informando a interrupção
                return;
            }

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
            }
            exclui_msg(resposta);
        }
        i++;
    }

    envia_ftx(socket);

    tentativas = 0;
    acabou = 0;
    while (acabou == 0) {
        if (interrompido) { // Verifica se houve interrupção
            printf("Interrupção detectada. Encerrando envio...\n");
            close(fd);
            envia_erro(socket, 0x02); // Opcional: Envia erro para o cliente informando a interrupção
            return;
        }

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
        }
    }

    printf("Conexão encerrada\n");
    exclui_janela(janela);
    close(fd); // Certifique-se de fechar o arquivo ao final
}


// Funcao que abre um video
void abre_video(const char *filename) 
{
    char comando[256];
    snprintf(comando, sizeof(comando), "xdg-open %s", filename);
    system(comando);
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

        int tempo = 0;
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
            // Processa ACK conforme necessário
            break;

        case NACK:
            printf("NACK recebido\n\n");
            // Processa NACK conforme necessário
            break;

        case FTX:
            printf("FTX recebido\n\n");
            // Processa FTX conforme necessário
            break;
        
        case ENCERRAMENTO: 
            printf("Recebido sinal de encerramento do cliente.\n");
            interrompido = 1; // Define a flag para sair do loop
            break;
        
        case ERRO:
            printf("ERRO recebido\n\n");
            // Processa ERRO conforme necessário
            break;
    }
}

void cuidar_erro(protocolo *msg){
    if (msg->tipo == ERRO){
        switch (msg->seq)
        {
        case 1:
            //printf(ERRO_ACESSO_NEGADO);
            printf("Erro no acesso ao arquivo\n");
            exclui_msg(msg);
            exit(1);
            break;
        case 2:
            printf(ERRO_NAO_ENCONTRADO);
            exclui_msg(msg);
            exit(2);
            break;
        case 3:
            printf(ERRO_DISCO_CHEIO);
            exclui_msg(msg);
            exit(3);
            break;
        default:
            printf(ERRO_PADRAO);
            exclui_msg(msg);
            exit(4);
            break;
        }
    }
}

char **cria_lista(int tamanho){
    char **lista = malloc(tamanho * sizeof(char*));
    if (lista == NULL)
        return NULL;
    for(int i = 0; i < tamanho; i++){
        lista[i] = malloc(64 * sizeof(char));
        if (lista[i] == NULL)
            return NULL;
    }
    return lista;
}

void exclui_lista(char **lista, int tamanho){
    for(int i = 0; i < tamanho; i++){
        if (lista[i])
            free(lista[i]);
    }
    free(lista);
}

void exclui_janela2(protocolo **janela){
    for(int i = 0; i < MAX_JANELA; i++){
        if (janela[i])
            exclui_msg(janela[i]);
    }
}

void exclui_janela(protocolo *janela[]) {
    for (int i = 0; i < MAX_JANELA; i++) {
        if (janela[i] != NULL) {
            exclui_msg(janela[i]); // Certifique-se de que exclui_msg não está liberando memória já liberada.
            janela[i] = NULL;
        }
    }
}
