#ifndef DEFINE_H
#define DEFINE_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <inttypes.h>
#include <sys/select.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include "raw_socket.h"
#include <arpa/inet.h>


//Minhas LIBs

/* ##################*/

/* ####  STRUCTS ####*/

typedef struct controle{
	char tipo;
	char interface[10];
}controle;

typedef struct protocolo{
	uint8_t inicio;
	uint8_t tam;
	uint8_t seq;
	uint8_t tipo;
	uint8_t *dados;
	uint8_t crc;
}protocolo;

#define MAX_JANELA 5

typedef struct {
	protocolo *buffer[MAX_JANELA]; // Mensagens na janela
	int base;						 // Base da janela
    int next_seq;					 // Próxima sequência a ser enviada
    int tamanho;					 // Tamanho da janela
} janela_deslizante;

/* ####   ERROS  ####*/
#define ERRO_ACESSO 0x3E	//111110

#define ERRO_PARAM "Parametro Invalido - Execução Abortada"
#define ERRO_ACESSO_NEGADO "Acesso Negado - Execução Abortada"
#define ERRO_PADRAO "Erro - Execução Abortada"
#define ERRO_NAO_ENCONTRADO "Video Nao Encontrado - Execução Abortada"
#define ERRO_DISCO_CHEIO "Disco esta cheio - Execução Abortada"

/* #### MENSAGEM ####*/
#define MSG_PARAMAJUDA "Utilize -h para ajuda!"

// Constante para o marcador de início do pacote
#define PACKET_START_MARKER 0x7E //01111110 126 em decimal
#define PACKET_DATA_MAX_SIZE 63 // Tamanho máximo dos dados
#define ERROPARID 0x3E	//111110 

#define ACK 0x00		  		//"00000"  0
#define NACK 0x01		  		//"00001"  1
#define LISTA 0x0A		  		//"01010"  10
#define BAIXAR 0x0B		  		//"01011"  11
#define MOSTRA_TELA 0x10  		//"10000"  12
#define DESCRITOR_ARQUIVO 0x11  //"10001"  13
#define DADOS 0x12		        //"10010"  18
#define FTX 0x1E		   		//"11110"  30
#define ERRO 0x1F		   		//"11111"  31
/* ########## COMANDOS ######### */
#define EXIT 0xFFFFFF
/* ############################ */

/* CLIENT */

/* SERVIDOR */

/* FUNCAO GERAIS */

// Função para verificar os parametros
void param(int argc, char *argv[],controle* parametro);

#endif  // DEFINE_H