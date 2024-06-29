#include "define.h"

const uint8_t crc8_table[256] = {
    0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
    0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
    0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
    0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
    0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
    0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
    0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
    0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
    0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
    0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
    0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
    0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
    0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
    0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
    0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
    0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};

uint8_t calc_crc8_with_table(uint8_t *msg, int tamBytes) 
{
    uint8_t crc = 0;

    for (int i = 0; i < tamBytes; i++) {
        crc = crc8_table[crc ^ msg[i]];
    }

    return crc;
}

uint8_t calc_crc8(protocolo *msg)
{
    uint8_t crc = 0xFF;  // Inicializa o CRC com 0xFF
    const uint8_t *ptr = (const uint8_t *)msg;  // Ponteiro para os dados da mensagem

    // Calcular o CRC-8 utilizando a tabela de lookup
    for (size_t i = 0; i < sizeof(protocolo); i++) {
        crc = crc8_table[crc ^ ptr[i]];
    }

    return crc;
}

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
    uint8_t* dados = (uint8_t*)malloc(tam + 1); // Alocar espaço para dados + 1 para o terminador
    if (!dados) {
        fprintf(stderr, "Falha ao alocar vetor de uint8_t - Dados ou buffer\n");
        exit(-1);
    }
    dados[tam] = '\0'; // Adicionar terminador nulo no final dos dados
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
    if (!msg) {
        return NULL;
    }

    msg->inicio = 126;
    msg->tam = tam;
    msg->seq = seq;
    msg->tipo = tipo;
    msg->dados = NULL;

    if (tam > 0 && dados != NULL) {
        msg->dados = (uint8_t *)malloc(tam);
        if (!msg->dados) {
            free(msg);
            return NULL;
        }
        memcpy(msg->dados, dados, tam);
    }

    msg->crc = calc_crc8(msg);
    return msg;
}

void timeout(int socket, protocolo *msg) 
{
    fd_set readset;
    struct timeval tv;
    int i = 0;
    int result;

    while (i < TENTATIVA_MAX) {
        tv.tv_sec = TEMPO_ESPERA;
        tv.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(socket, &readset);

        result = select(socket + 1, &readset, NULL, NULL, &tv);

        if (result <= 0) { // Timeout
            printf("TIMEOUT - Tentativa %d\n", i + 1);
            envia_msg(socket, msg);
            i++;
        } else if (FD_ISSET(socket, &readset)) {
            // Recebeu resposta, sair do loop
            break;
        }
    }

    if (i == TENTATIVA_MAX) {
        printf("ERRO - Timeout máximo atingido. Abortando operação!\n");
        exit(0);
    }
}

void timeout_dados(int socket, protocolo *msg1, protocolo *msg2) 
{
    fd_set readset;
    struct timeval tv;
    int i = 0;
    int result;

    while (i < TENTATIVA_MAX) {
        tv.tv_sec = TEMPO_ESPERA;
        tv.tv_usec = 0;

        FD_ZERO(&readset);
        FD_SET(socket, &readset);

        result = select(socket + 1, &readset, NULL, NULL, &tv);

        if (result <= 0) { // Timeout
            printf("TIMEOUT - Tentativa %d\n", i + 1);
            envia_msg(socket, msg1);
            envia_msg(socket, msg2);
            i++;
        } else if (FD_ISSET(socket, &readset)) {
            // Recebeu resposta, sair do loop
            break;
        }
    }

    if (i == TENTATIVA_MAX) {
        printf("ERRO - Timeout máximo atingido. Abortando operação!\n");
        exit(0);
    }
}

void envia_msg(int socket,protocolo *msg)
{
	uint8_t *buffer;
	int i=0,tam_buffer;
	
	if(msg)
    {
		tam_buffer = 67;
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
		for(i=0;i<msg->tam;i++){
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

protocolo* recebe_msg(int socket,int n_msgs)
{
	protocolo *msg;
	uint8_t *buffer;
	int i,tam_buffer;
	
	//4bytes fixo + 63 bytes dados max
	tam_buffer = 67;
	//aloca buffer
	buffer = aloca_vetor(tam_buffer);
	//aloca msg
	msg = aloca_msg();
	//recebe msg e salva no buffer
	//read(socket,buffer,tam_buffer);
    int bytes_read = read(socket, buffer, tam_buffer);
	if(buffer[0] == PACKET_START_MARKER)
    {
		//inicio
		msg->inicio = buffer[0];
		//tamanho
		msg->tam = (buffer[1] >> 2);
		//aloca vetor DADOS
		msg->dados = aloca_vetor(msg->tam);
		//sequencia
		msg->seq = (buffer[1] << 6);
		msg->seq = (msg->seq >> 3); // acerta 3 bits mais significativos em 0
		msg->seq |= (buffer[2] >> 5);
		//tipo
		msg->tipo = (buffer[2] << 3);
		msg->tipo = (msg->tipo >> 3);
		//dados
		for(i=0;i<msg->tam;i++){
			msg->dados[i] = buffer[i+3];
		}
		//crc
        uint8_t received_crc = buffer[bytes_read - 1];
        msg->crc = calc_crc8_with_table(buffer, bytes_read - 1);

        // Verifica se o CRC recebido é válido
        if (msg->crc != received_crc) 
        {
            fprintf(stderr, "ERRO: CRC inválido na mensagem recebida.\n");
            free(msg->dados);
            free(msg);
            return NULL; // Retorna NULL em caso de CRC inválido
        }

        /*
        if (msg->crc != calc_crc8(msg)) {
            printf("ERRO - CRC Incorreto\n");
            return NULL;
        }
        */

		//libera BUFFER
		free(buffer);
        //printf("Mensagem recebida: inicio=%d, tam=%d, seq=%d, tipo=%d, crc=%d\n", msg->inicio, msg->tam, msg->seq, msg->tipo, msg->crc);
		return (msg);
	}else{
		//lixo
		return(NULL);
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
	int i;
	for(i=0;i<tam;i++){
		buffer[i] = 0;
	}
	
}

void d_erro(uint8_t *codigo)
{
	switch(codigo[0]){
		case 0:
			fprintf(stderr, "bash: : Arquivo ou diretório não encontrado\n");
			break;
		case 1:
		    fprintf(stderr, "bash: cd: Permissão negada\n");
			break;
		case 2:
			fprintf(stderr, "Symbolic link loop.\n");
			break;
		case 3:
		    fprintf(stderr, "bash: cd: : Nome de arquivo muito longo\n");
		    break;
		case 4:
		    fprintf(stderr, "bash: cd: : Não é um diretório\n");
		    break;
		default:
			printf("ERRO NÂO CONHECIDO\n");
			break;
	}
}

void imprime_msg(protocolo *msg)
{
	if(msg){
		printf("############################\n");
		printf("INICIO : %d \n",msg->inicio);
		printf("TAM : %d \n",msg->tam);
		printf("SEQ : %d \n",msg->seq);
		printf("TIPO : %d \n",msg->tipo);
		printf("DADOS : %s \n",(char*)msg->dados);
        printf("CRC : %d \n",msg->crc);
        printf("\n");
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