#include "janela.h"

// Caso que o cliente manda uma mensagem do tipo lista "01010"
// Servidor pode responder com ACK? , NACK, MOSTRA_TELA ou ERRO
// Cliente responde com ACK ou NACK ate o servidor mande FTX(fim transmissao) e o cliente responda com ACK ou NACK

// Caso que o cliente manda uma mensagem do tipo baixar "01011" e o servidor responde com ACK? , NACK , ERRO ou DESCRITROR_ARQUIVO
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
        printf("Mensagem do tipo 'pedido da lista' enviada para o servidor.\n");

        if (espera(socket, 2) == 1) {
            printf("Timeout pedindo lista - tentando novamente - %d\n", tentativas + 1);
            tentativas++;
        } else {
            protocolo *resposta = recebe_msg(socket, 1);
            if (resposta != NULL && resposta->inicio == PACKET_START_MARKER) {
                if (resposta->tipo == ACK || resposta->tipo == MOSTRA_TELA) {

                    if (resposta->tipo == ACK) {
                        printf("recebeu ACK\n");
                        imprime_msg(resposta);
                        exclui_msg(resposta);
                        espera(socket, -1);
                        resposta = recebe_msg(socket, 1);
                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && resposta->tipo == MOSTRA_TELA) {
                            printf("recebeu MOSTRA_TELA\n");
                            imprime_msg(resposta);
                        } else {
                            imprime_msg(resposta);
                            exclui_msg(resposta);
                            printf("pacote recebido invalido\n");
                            return;
                        }
                    } else {
                        printf("recebeu MOSTRA_TELA direto\n");
                    }

                    tentativas = 0;
                    protocolo *janela[MAX_JANELA];
                    memset(janela, 0, MAX_JANELA * sizeof(protocolo*));
                    char **lista = NULL;
                    int seq_esperado = 0;
                    // enquanto nao acabar a transmissao
                    while (resposta == NULL || resposta->tipo != FTX) {
                        if (resposta != NULL && resposta->inicio == PACKET_START_MARKER && (resposta->tipo == MOSTRA_TELA || resposta->tipo == FTX)){
                            printf("pacote VALIDO\n");
                            imprime_msg(resposta);
                            if (resposta->tipo == ERRO)
                                cuidar_erro(resposta);

                            // pacote recebido eh o esperado
                            if (resposta->seq == (seq_esperado%32) && resposta->tipo == MOSTRA_TELA) {
                                janela[0] = resposta; // 0 eh a resposta
                                int i = 1; // quantidade de pacotes a processar
                                while (janela[i] != NULL && i < MAX_JANELA) {
                                    if (janela[i]->seq != ((seq_esperado + i)%32)) { // teste janela do cliente
                                        printf("seq esperado = %d; i = %d; janela[i]->seq = %d\n", seq_esperado%32, i, janela[i]->seq);
                                        printf("ERRO NA JANELA DO CLIENTE\n\n");
                                    }
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
                                printf("ENVIANDO ACK %d\n", (seq_esperado+i-1)%32);
                                //limite de seq eh 32
                                seq_esperado += i;
                            } else { //nao eh o esperado
                                printf("ENVIANDO NACK %d\n", seq_esperado%32);
                                envia_nack(socket, seq_esperado%32);
                                int diferenca = resposta->seq - seq_esperado%32;
                                if (diferenca > 0 && diferenca < MAX_JANELA)
                                    janela[diferenca] = resposta;
                            }
                        }
                        else{
                            printf("pacote INVALIDO\n");
                            if (resposta != NULL){
                                printf("tentando imprimir msg\n");
                                imprime_msg(resposta);
                            } 
                        }

                        //espera pelo proximo pacote
                        printf("esperando proximo ou FTX\n");
                        if (socket > 1024){
                            printf("socket = %d\n", socket);
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
                    printf("recebeu FTX\n");
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
                                printf("RECEBEU FTX NOVAMENTE\n");
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
                        printf("recebeu NACK\n");
                        tentativas++;
                    }
                    if (resposta->tipo == ERRO) {
                        printf("recebeu ERRO\n");
                        tentativas++;
                    }
                    if (resposta->tipo == LISTA)
                        printf("recebeu proprio pedido LISTA; ignorado\n");
                }
            } else {
                printf("Pacote recebido invalido\n");
            }
        }
    }
    printf("Conexão falhou depois de %d tentativas (timeout)\n", tentativas);
    return;
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
    //envia_mensagem_com_janela(socket, msg, janela);
    imprime_msg(msg);
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
            switch (resposta->tipo) 
            {
                case DADOS:
                    fwrite(resposta->dados, 1, resposta->tam, arquivo);
                    imprime_msg(resposta);
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
        else{
            printf("pacote recebido invalido\n");
            return;
        }
    }

    fclose(arquivo);
    printf("Arquivo %s baixado com sucesso.\n", filename);

    // Chama o reprodutor de vídeo após o download do arquivo
    abre_video(filename);
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

    printf("qnt_arquivos = %d\n", qnt_arquivos);
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
        printf("i = %d\n", i);
        printf("inicio_janela = %d\n", inicio_janela);
        printf("fim_janela = %d\n", fim_janela);
        // Se chegou no fim da janela, envia ela novamente
        if (i == fim_janela + 1){ //mais 1 para mandar o ultimo
            i = inicio_janela;
            tentativas++;
        }
        if (tentativas == 16){
            printf("Timeout, sem resposta do cliente apos 16 tentativas\n");
            return;
        }

        printf("enviando dado janela[%d] = %p\n", (i+(MAX_JANELA-1))-fim_janela, janela[(i+(MAX_JANELA-1))-fim_janela]);
        imprime_msg(janela[(i+(MAX_JANELA-1))-fim_janela]);
        envia_msg(socket, janela[(i+(MAX_JANELA-1))-fim_janela]);

        int tempo = 0;
        if (tentativas == 9) //se janela deu 10 voltas sem receber, espera 1 seg para receber
            tempo = 1;
        while (espera(socket, tempo) == 0) { // Se chegou um pacote
            tentativas = 0;
            protocolo *resposta = recebe_msg(socket, 1);
            if (resposta == NULL || resposta->inicio != PACKET_START_MARKER) {
                exclui_msg(resposta);
                printf("Pacote recebido inválido\n");
                break;
            }

            if (resposta->tipo == ACK || resposta->tipo == NACK) {
                printf("RECEBEU %s %d\n", resposta->tipo == ACK ? "ACK" : "NACK", resposta->seq);
                if (resposta->tipo == NACK && resposta->seq == 5){
                    printf("aa\n");
                }
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
                            printf("ACK incorreto recebido\n");
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

// Função para enviar um arquivo ao cliente
// void envia_arquivo(int socket, const char *filename) 
// {
//     char filepath[256];
//     snprintf(filepath, sizeof(filepath), "%s/%s", VIDEO_DIR, filename);
//     printf("Tentando abrir o arquivo: %s\n", filepath);

//     int fd = open(filepath, O_RDONLY);
//     if (fd < 0) {
//         perror("Erro ao abrir arquivo para leitura");
//         envia_erro(socket, 0x01); // Acesso negado
//         return;
//     }

//     struct stat st;
//     if (fstat(fd, &st) < 0) {
//         perror("Erro ao obter informações do arquivo");
//         close(fd);
//         envia_erro(socket, 0x01);
//         return;
//     }

//     size_t tamanho = st.st_size;
//     char descritor[128];
//     snprintf(descritor, sizeof(descritor), "tamanho=%zu,data=20230601", tamanho);

//     protocolo *descritor_msg = cria_msg(0, DESCRITOR_ARQUIVO, (uint8_t *)descritor, strlen(descritor));
//     envia_msg(socket, descritor_msg);
//     free(descritor_msg);

//     uint8_t buffer[PACKET_DATA_MAX_SIZE];
//     ssize_t bytes_lidos;

//     while ((bytes_lidos = read(fd, buffer, sizeof(buffer))) > 0) {
//         protocolo *dados_msg = cria_msg(0, DADOS, buffer, bytes_lidos);
//         envia_msg(socket, dados_msg);
//         free(dados_msg);
//     }

//     close(fd);

//     protocolo *ftx_msg = cria_msg(0, FTX, NULL, 0);
//     envia_msg(socket, ftx_msg);
//     free(ftx_msg);
// }

// Encher como lixo se for usar menos de 63 bytes
// Função para mandar os arquivos em partes
void envia_partes_arquivo(int socket, uint8_t seq_inicial, const char *arquivo) 
{
    printf("Dentro da funcao envia_partes_arquivo\n");
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

    while ((bytes_lidos = fread(buffer, 1, sizeof(buffer), fp)) > 0) 
    {
        protocolo *dados_msg = cria_msg(seq, DADOS, buffer, bytes_lidos);
        imprime_msg(dados_msg);
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
    imprime_msg(msg); //imprime detalhes da mensagem

    switch (msg->tipo)
    {
        case LISTA:
            printf("Pedido de lista recebido\n\n");

            // Verifica se a mensagem é válida e envia a resposta correspondente
            //CHECAR COM CRC SE EH VALIDA
                // printf("Mensagem válida\n");
            lista_arquivos(socket);

            // } else {
            //     // Se a mensagem for inválida, envie NACK
            //     envia_nack(socket, 0);
            // }
            break;

        case BAIXAR:
            printf("Pedido de download recebido\n\n");
            //envia_ack(socket, msg->seq);
            envia_partes_arquivo(socket, msg->seq + 1, (char *)msg->dados);
            break;

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
        
        case ERRO:
            printf("ERRO recebido\n\n");
            // Processa ERRO conforme necessário
            break;
        
        //default:
          //  printf("Tipo de mensagem não reconhecido: %d\n", msg->tipo);
            //envia_erro(socket, msg->seq);
            //break;
    }
}

void cuidar_erro(protocolo *msg){
    if (msg->tipo == ERRO){
        switch (msg->dados[0])
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

void exclui_janela(protocolo **janela){
    for(int i = 0; i < MAX_JANELA; i++){
        if (janela[i])
            exclui_msg(janela[i]);
    }
}