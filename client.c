#include "raw_socket.h"
#include "janela.h"
#include "protocolo.h"
#include <signal.h>
#include <setjmp.h>

static jmp_buf env;

// Definir um manipulador de sinal para SIGINT
void handle_sigint(int sig) {
    printf("\nInterrupção detectada. Retornando ao menu principal...\n");
    longjmp(env, 1);
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
                printf("Para fazer uma interrupção digite ctrl c");
                return;
            default:
                printf("Opção inválida. Tente novamente.\n");
        }
    }
}


int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);

    int rsocket;
    char interface[] = "eth0"; // Interface de rede (loopback)
    rsocket = abrirRawSocket(interface);
    if (rsocket < 0) {
        fprintf(stderr, "Erro ao abrir socket RAW.\n");
        exit(1);
    }

    if (setjmp(env) == 0) {
        // Executa normalmente
    } else {
        // Executa após uma interrupção
        printf("Retornando ao menu principal...\n");
        envia_erro(rsocket,0);
    }

    printf("Socket RAW aberto com sucesso na interface: %s\n", interface);

    interface_cliente(rsocket);

    close(rsocket);
    return 0;
}