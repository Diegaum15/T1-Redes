#include "define.h"
#include "crc.h"

void param(int argc, char *argv[],controle* parametro)
{
	char option;
	int cont=2;
	
	while ((option = getopt(argc, argv,"tih")) != -1){
		switch (option){
			case 't':
				parametro->tipo = *argv[cont]; 
				break;
			case 'i':
				strcpy(parametro->interface,argv[cont]);
				break;
			case 'h':
				break;
			default:
				printf(ERRO_PARAM"\n");
				printf(MSG_PARAMAJUDA"\n");
				exit(1);
				break;
		}
		cont+=2;
	}
}

/*
    Sugestoes de melhorias: 

    Adicionar mais verificações de erro e logs nas funções para facilitar o debugging.
    Usar sizeof(protocolo) de maneira segura para evitar erros de buffer.
    Modularizar o código em arquivos separados (protocolo.c, crc.c, etc.) para facilitar a manutenção.
*/

/*

protocolo* cria_msg2(uint8_t seq, uint8_t tipo, uint8_t* dados, ...) 
{
    protocolo* msg = aloca_msg();
    va_list arg;
    va_start(arg, dados);

    msg->dinicio = PACKET_START_MARKER;
    msg->seq = seq;
    msg->tipo = tipo;

    if (tipo == DADOS) {
        msg->tam = va_arg(arg, int);
    } else if (tipo == NACK || tipo == ERRO) {
        msg->tam = sizeof(uint8_t);
    } else if (dados) {
        msg->tam = strlen((char*)dados);
    } else {
        // Sem dados na mensagem
        const char* defaultData = "#N#NULL#N#NULL";
        msg->tam = strlen(defaultData);
        dados = (uint8_t*)defaultData;
    }

    msg->dados = aloca_vetor(msg->tam);
    memcpy(msg->dados, dados, msg->tam); // Copia os dados para a mensagem
    msg->crc = calc_crc8(msg); // Calcular CRC da mensagem

    va_end(arg);
    return msg;
}


void envia_msg(int socket, protocolo *msg) 
{
    uint8_t *buffer;
    int tam_buffer = 4 + msg->tam + 1; // Ajustando para o tamanho correto
    int i;

    if (!msg) 
    {
        fprintf(stderr, "ERRO - Enviar mensagem vazia.\n");
        return;
    }

    buffer = aloca_vetor(tam_buffer);
    if (!buffer) {
        fprintf(stderr, "ERRO - Falha ao alocar buffer.\n");
        return;
    }

    // Preencher o buffer com os dados da mensagem
    buffer[0] = msg->dinicio; // Marcador de início
    printf("Marcador de inicio %d \n", buffer[0]);
    buffer[1] = (msg->tam << 2) | (msg->seq >> 8); // Tamanho (parte alta)
    buffer[2] = msg->seq; // Sequência (parte baixa)
    buffer[3] = msg->tipo; // Tipo

    // Copiar os dados para o buffer
    for (i = 0; i < msg->tam; i++) {
        buffer[4 + i] = msg->dados[i];
    }

    // Calcular o CRC para os bytes relevantes da mensagem
    msg->crc = calc_crc8_with_table(buffer, tam_buffer - 1);

    // Incluir o valor do CRC no último byte do buffer
    buffer[tam_buffer - 1] = msg->crc;

    // Enviar o buffer pelo socket
    write(socket, buffer, tam_buffer);

    free(buffer);
}


protocolo* recebe_msg(int socket, int n_msgs)
{
    protocolo *msg = NULL;
    uint8_t *buffer = NULL;
    int tam_buffer = 67;

    printf("Dentro da funcao recebe msg\n");

    // Aloca o buffer para receber a mensagem
    buffer = aloca_vetor(tam_buffer);
    printf("Buffer alocado\n");
    if (!buffer) {
        fprintf(stderr, "Erro ao alocar memória para o buffer da mensagem.\n");
        return NULL;
    }
    printf("Buffer alocado com sucesso\n");
    
    // Recebe a mensagem pelo socket e salva no buffer
    int bytes_read = read(socket, buffer, tam_buffer);
    printf("Bytes lidos: %d\n", bytes_read);
    if (bytes_read < 0) {
        fprintf(stderr, "Erro ao receber a mensagem do socket: %s\n", strerror(errno));
        free(buffer);
        return NULL;
    }

    // Verifica se nenhum byte foi lido
    if (bytes_read == 0) {
        fprintf(stderr, "Erro: Nenhum byte lido do socket.\n");
        free(buffer);
        return NULL;
    }

    
    printf("\n");
    printf("Mensagem recebida dentro da funcao recebe_msg.\n");
    printf("Buffer recebido (byte a byte):\n");
    for (int i = 0; i < bytes_read; i++) {
        printf("%d ", buffer[i]);
    }
    
    printf("\n");
    //printf("Tamanho do buffer: %d\n", bytes_read);
    //printf("Marcador de inicio: %d\n", buffer[0]);
    //printf("Tamanho: %d\n", buffer[1]);
    //printf("Sequência: %d\n", buffer[2]);
    //printf("Tipo: %d\n", buffer[3]);
    //printf("Dados: %s\n", (char*)buffer + 4);

    // Verifica o marcador de início da mensagem
    if (buffer[0] != PACKET_START_MARKER) {
        fprintf(stderr, "ERRO: Marcador de início inválido na mensagem recebida.\n");
        free(buffer);
        return NULL;
    }

    // Calcula o tamanho esperado da mensagem
    int tamanho_esperado = (buffer[1] >> 2) + 4 + 1; // Tamanho + cabeçalho (4 bytes) + CRC (1 byte)
    printf("Tamanho esperado: %d\n", tamanho_esperado);

    // Verifica o tamanho da mensagem recebida
    if (tamanho_esperado != bytes_read) {
        fprintf(stderr, "ERRO: Tamanho da mensagem recebida não corresponde ao esperado.\n");
        free(buffer);
        return NULL;
    }

    // Aloca a estrutura da mensagem
    msg = aloca_msg();
    if (!msg) {
        fprintf(stderr, "Erro ao alocar memória para a mensagem.\n");
        free(buffer);
        return NULL;
    }

    // Preenche a estrutura da mensagem com os dados do buffer
    msg->dinicio = buffer[0]; // Marcador de início
    msg->tam = (buffer[1] >> 2); // Tamanho
    msg->seq = ((buffer[1] & 0x03) << 8) | buffer[2]; // Sequência
    msg->tipo = buffer[3]; // Tipo

    // Aloca espaço para os dados da mensagem
    msg->dados = aloca_vetor(msg->tam);
    if (!msg->dados) {
        fprintf(stderr, "Erro ao alocar memória para os dados da mensagem.\n");
        free(msg);
        free(buffer);
        return NULL;
    }

    // Copia os dados do buffer para a mensagem
    for (int i = 0; i < msg->tam; i++) {
        msg->dados[i] = buffer[4 + i];
    }

    // Verifica o CRC recebido
    uint8_t received_crc = buffer[bytes_read - 1];
    msg->crc = calc_crc8_with_table(buffer, bytes_read - 1);

    // Libera o buffer
    free(buffer);

    // Verifica se o CRC recebido é válido
    if (msg->crc != received_crc) {
        fprintf(stderr, "ERRO: CRC inválido na mensagem recebida.\n");
        free(msg->dados);
        free(msg);
        return NULL; // Retorna NULL em caso de CRC inválido
    }

    return msg; // Retorna a mensagem válida
}
*/