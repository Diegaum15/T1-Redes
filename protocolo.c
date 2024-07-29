#include "protocolo.h"

void exclui_msg(protocolo *msg) {
    if (msg) {
        if (msg->dados) {
            free(msg->dados);
        }
        free(msg);
    }
}

uint8_t* aloca_vetor(int tam) 
{
    uint8_t* dados = (uint8_t*)malloc(tam); // Alocar espaço para dados
    if (!dados) {
        fprintf(stderr, "Falha ao alocar vetor de uint8_t - Dados ou buffer\n");
        exit(-1);
    }
    //dados[tam] = '\0'; // Adicionar terminador nulo no final dos dados
    memset(dados, 0, tam); // Inicializa o vetor alocado com zeros
    return dados;
}

protocolo* aloca_msg() 
{
    protocolo* msg = (protocolo*)malloc(sizeof(protocolo));
    if (!msg) {
        fprintf(stderr, "Falha ao alocar mensagem\n");
        exit(-1);
    }
    memset(msg, 0, sizeof(protocolo)); // Inicializa a mensagem com zeros
    return msg;
}

uint8_t cal_seq(protocolo *msg)
{
	uint8_t new_seq;
	//5bit 0-31 seq
	if(!msg)
		return (0);
	if(msg->seq < 31){
		new_seq = msg->seq+1;
	}else{
		new_seq = 0;
	}
	return (new_seq);
}

protocolo* cria_msg(uint8_t seq, uint8_t tipo, const uint8_t *dados, size_t tam) 
{
    protocolo *msg = (protocolo *)malloc(sizeof(protocolo));
    if (!msg)
        return NULL;

    msg->inicio = PACKET_START_MARKER;
    msg->tam = tam;
    msg->seq = seq;
    msg->tipo = tipo;
    msg->dados = (uint8_t *)malloc(64);
    if (!msg->dados) {
        free(msg);
        return NULL;
    }

    if (tam > 0) 
        memcpy(msg->dados, dados, tam);
    padding_dados(msg->dados, tam);

    msg->crc = 0;
    return msg;
}


int espera(int socket, int timeout){
    fd_set readfds;
    struct timeval tv;

    FD_ZERO(&readfds);
    FD_SET(socket, &readfds);

    //se for maior que 1023 select não funciona
    if (socket >= 1024){
        printf("ERRO - limite de socket eh 1024\n");
        return 1;
    }

    //monitora fd(no caso socket)
    int ret;
    if (timeout < 0) {
        ret = select(socket + 1, &readfds, NULL, NULL, NULL);
    } else {
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        ret = select(socket + 1, &readfds, NULL, NULL, &tv);
    }

    if (ret == -1){
        printf("Erro lendo socket\n");
        return -1;
    }

    //testa se recebeu algo
    if (ret > 0 && FD_ISSET(socket, &readfds))
        return 0;
    else
        return 1;
}

void envia_msg(int socket, protocolo *msg)
{
	uint8_t *buffer;
	int i=0,tam_buffer = 67;
	
	if(msg)
    {
		buffer = aloca_vetor(tam_buffer);
		//inicio
		buffer[0] = msg->inicio;
		//tamanho
		buffer[1] = (msg->tam << 2);
		//Sequencia
		buffer[1] |= (msg->seq >> 3); // 2 ultimos bits
		buffer[2] = (msg->seq << 5); // 3 bits o inicio
		//tipo
		buffer[2] |= (msg->tipo); // não precisa desloca
		//dados
		for(i=0;i<64;i++){
			buffer[3+i] = msg->dados[i];
		}
		//crc
        //printf("Enviando mensagem: inicio=%d, tam=%d, seq=%d, tipo=%d, crc=%d\n", msg->inicio, msg->tam, msg->seq, msg->tipo, msg->crc);

        // Calcular o CRC para os bytes relevantes da mensagem
        msg->crc = calc_crc8_with_table(buffer, tam_buffer - 1);

        // Incluir o valor do CRC no último byte do buffer
        buffer[tam_buffer - 1] = msg->crc;
		//Enviar Buffer
		write(socket,buffer,tam_buffer);
		free(buffer);
	}else{
		fprintf(stderr,"ERRO - Enviar MSG Vazia");
	}
}

//MUDAR A IMPLEMENTAÇÃO DO CRC, calcular em cima com o crc recebido e comparar com 0
// n_msgs??
protocolo* recebe_msg(int socket, int n_msgs) {
    protocolo *msg;
    uint8_t *buffer;
    int i, tam_buffer;

    // Tamanho fixo de 67 bytes
    tam_buffer = 67;

    // Aloca buffer
    buffer = (uint8_t*)malloc(tam_buffer);
    if (!buffer) {
        fprintf(stderr, "Falha ao alocar buffer\n");
        exit(-1);
    }

    // Aloca mensagem
    msg = (protocolo*)malloc(sizeof(protocolo));
    if (!msg) {
        fprintf(stderr, "Falha ao alocar mensagem\n");
        free(buffer);
        exit(-1);
    }

    // Inicializa o buffer da mensagem
    memset(buffer, 0, tam_buffer);
    memset(msg, 0, sizeof(protocolo));

    // Recebe mensagem e salva no buffer
    int bytes_read = read(socket, buffer, tam_buffer);
    if (bytes_read <= 0) {
        free(buffer);
        free(msg);
        return NULL;
    }

    if (buffer[0] == PACKET_START_MARKER) {
        // Inicio
        msg->inicio = buffer[0];
        // Tamanho
        msg->tam = (buffer[1] >> 2);

        // Aloca vetor DADOS
        msg->dados = (uint8_t*)malloc(64);
        if (!msg->dados) {
            fprintf(stderr, "Falha ao alocar vetor de dados\n");
            free(buffer);
            free(msg);
            exit(-1);
        }
        memset(msg->dados, 0, 64);

        // Sequência
        msg->seq = (buffer[1] << 6);
        msg->seq = (msg->seq >> 3); // Acessa os 3 bits mais significativos
        msg->seq |= (buffer[2] >> 5);

        // Tipo
        msg->tipo = (buffer[2] << 3);
        msg->tipo = (msg->tipo >> 3);

        // Dados
        for (i = 0; i < 64; i++) {
            msg->dados[i] = buffer[i + 3];
        }

        // CRC
        uint8_t received_crc = buffer[bytes_read - 1];
        msg->crc = calc_crc8_with_table(buffer, bytes_read - 1);

        // Verifica se o CRC recebido é válido
        if (msg->crc != received_crc) {
            fprintf(stderr, "ERRO: CRC inválido na mensagem recebida.\n");
            free(msg->dados);
            free(msg);
            free(buffer);
            return NULL;
        }

        // Libera buffer
        free(buffer);
        return msg;
    } else {
        // Lixo
        free(buffer);
        free(msg);
        return NULL;
    }
}


uint8_t ler_msg(protocolo *msg) 
{
    printf("Entrou na função ler_msg\n");
    printf("\n");
    if (!msg) 
    {
        printf("ERRO - Mensagem inválida entrou no if !msg \n");
        return ERRO_ACESSO; // Mensagem inválida
    }

    printf("Marcador de inicio: %d\n", msg->inicio);
    // Verifica se o marcador de início é válido
    if (msg->inicio == PACKET_START_MARKER) 
    {
        return msg->tipo; // Retorna o tipo da mensagem
    } else 
    {
        printf("ERRO - Marcador de início inválido\n");
        return ERRO_ACESSO; // Marcador de início inválido
    }
}

void limpa_buffer(char *buffer,int tam)
{
	memset(buffer, 0, 64 - tam);
}

void imprime_msg(protocolo *msg)
{
	if(msg){
		printf("############################\n");
		printf("INICIO : %d \n",msg->inicio);
		printf("TAM : %d \n",msg->tam);
		printf("SEQ : %d \n",msg->seq);
		printf("TIPO : %d \n",msg->tipo);
        printf("DADOS :");
        for (int i = 0; i < 63; i++){
		    printf("%d ",msg->dados[i]);
        }
        printf("\nCRC : %d \n",msg->crc);
	}
}

void padding_dados(uint8_t *dados, size_t tam) 
{
    memset(dados + tam, 0xFF, 64 - tam); // Preenche o restante dos dados com 1s
}

void envia_pedido_lista(int socket) 
{
    protocolo *lista_msg = cria_msg(0, LISTA, NULL, 0);
    envia_msg(socket, lista_msg);
    exclui_msg(lista_msg);
}

void envia_ack(int socket, uint8_t seq) 
{
    protocolo *ack_msg = cria_msg(seq, ACK, NULL, 0);
    envia_msg(socket, ack_msg);
    exclui_msg(ack_msg);
}

void envia_nack(int socket, uint8_t seq) 
{
    protocolo *nack_msg = cria_msg(seq, NACK, NULL, 0);
    envia_msg(socket, nack_msg);
    exclui_msg(nack_msg);
}

void envia_ftx(int socket) 
{
    protocolo *ftx_msg = cria_msg(0, FTX, NULL, 0);
    envia_msg(socket, ftx_msg);
    exclui_msg(ftx_msg);
}

void envia_erro(int socket, uint8_t seq) 
{
    protocolo *erro_msg = cria_msg(seq, ERRO, NULL, 0);
    envia_msg(socket, erro_msg);
    exclui_msg(erro_msg);
}

