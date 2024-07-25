#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define VIDEO_DIR "/home/diegaum/Redes/code/videos" // Caminho absoluto do diretório // Diretório onde os vídeos estão armazenados
#define SERVER_PORT 12347

// Definições e implementações das estruturas e funções de protocolo
#include "protocolo.h"
#include "janela.h"
#include "raw_socket.h"
#include "protocolo.h"

int main() 
{
    int rsocket;
    char interface[] = "eth0"; // Interface de rede (loopback)
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Servidor iniciado na interface: %s\n", interface);

    while (1) {
        if (espera(rsocket, -1) == 0){// evita busy waiting
            protocolo *msg = recebe_msg(rsocket, 1);
            if (msg) {
                processa_mensagem_cliente(rsocket, msg);
                exclui_msg(msg);
            } else {
                fprintf(stderr, "Erro ao receber mensagem do cliente.\n");
            }
        }
    }

    close(rsocket);
    return 0;
}



/* Programa que testou a comunicacao entre cliente e servidor por go-back-n
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

    while (1) {
        protocolo *msg = recebe_msg(rsocket, 1);
        if (msg) {
            printf("Mensagem recebida do cliente:\n");
            imprime_msg(msg);

            processa_mensagem_cliente(rsocket, msg);
            exclui_msg(msg);
        } else {
            fprintf(stderr, "Erro ao receber mensagem do cliente.\n");
        }
    }

    close(rsocket);
    return 0;
}
*/

/* programa que testou para ver ser chegou todos os tipos de mensagem
int main() {
    int rsocket;
    char interface[] = "lo";
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);

    while (1) {
        protocolo *msg_recebida = recebe_msg(rsocket, 1);
        printf("Mensagem recebida do cliente:\n");
        if (!msg_recebida) 
        {
            fprintf(stderr, "Erro ao receber mensagem do cliente.\n");
            continue;
        }

        printf("Mensagem recebida do cliente:\n");
        imprime_msg(msg_recebida);

        processa_mensagem(rsocket, msg_recebida);
        exclui_msg(msg_recebida);
    }

    close(rsocket);
    return 0;
}
*/
/* programa que testou para ver ser chegou todos os tipos de mensagem
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

    while (1) {
        protocolo *msg_recebida = recebe_msg(rsocket, 1);
        if (!msg_recebida) {
            fprintf(stderr, "Erro ao receber mensagem do cliente.\n");
            continue;
        }

        printf("Mensagem recebida do cliente:\n");
        imprime_msg(msg_recebida);

        processa_mensagem(rsocket, msg_recebida);
        exclui_msg(msg_recebida);
    }

    close(rsocket);
    return 0;
}


// teste das funcoes de janela
int main() {
    int rsocket;
    char interface[] = "lo";
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);

    while (1) {
        protocolo *mensagem_recebida = recebe_msg(rsocket, 1);
        if (!mensagem_recebida) {
            fprintf(stderr, "Erro ao receber mensagem do cliente.\n");
            continue;
        }

        uint8_t tipo_msg = ler_msg(mensagem_recebida);
        printf("Tipo da mensagem recebida: %d\n", tipo_msg);
        printf("Dados da mensagem recebida: %s\n", mensagem_recebida->dados);

        if (tipo_msg == DADOS) {
            enviar_ack(rsocket, mensagem_recebida->seq);
        } else {
            enviar_nack(rsocket, mensagem_recebida->seq);
        }

        free(mensagem_recebida->dados);
        free(mensagem_recebida);
    }

    close(rsocket);
    return 0;
}
*/
/* Teste que funcionou do envio pelo socket
#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Receber dados do cliente
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received < 0) {
        perror("Erro ao receber dados do cliente");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    buffer[bytes_received] = '\0';
    printf("Mensagem recebida do cliente: %s\n", buffer);

    // Enviar uma resposta de volta ao cliente
    const char *response = "Mensagem recebida com sucesso!";
    send(client_socket, response, strlen(response), 0);

    // Fechar o socket do cliente
    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Criar o socket do servidor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao abrir o socket do servidor");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // Loopback address
    server_addr.sin_port = htons(SERVER_PORT);

    // Fazer o bind do socket do servidor ao endereço
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind do socket do servidor");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Colocar o socket em modo de escuta (listen)
    if (listen(server_socket, 5) < 0) {
        perror("Erro ao colocar o socket em modo de escuta");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Servidor aguardando conexões na porta %d...\n", SERVER_PORT);

    // Aguardar por conexões dos clientes
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Erro ao aceitar conexão do cliente");
            continue;
        }

        printf("Conexão estabelecida com um cliente\n");

        // Tratar a conexão com o cliente em um novo processo ou thread
        handle_client(client_socket);
    }

    // Fechar o socket do servidor
    close(server_socket);

    return 0;
}
*/

