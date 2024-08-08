// Definições e implementações das estruturas e funções de protocolo
#include "protocolo.h"
#include "janela.h"
#include "raw_socket.h"
#include <signal.h>
#include <setjmp.h>

volatile sig_atomic_t interrompido = 0; // Variável global para sinal de interrupção
jmp_buf env;

void handle_sigint_server(int sig) {
    interrompido = 1;
    longjmp(env, 1); // Volta para o ponto em que setjmp foi chamado
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