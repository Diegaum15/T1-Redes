#include "define.h"

int abrirRawSocket(char *interface) 
{
    int rsocket;
    struct packet_mreq mreq;
    struct ifreq ifr;
    struct sockaddr_ll local;

    // Abrir o socket RAW
    if ((rsocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        perror("Erro ao abrir socket RAW");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, interface);

    // Obter o índice da interface
    if (ioctl(rsocket, SIOCGIFINDEX, &ifr) < 0) {
        perror("Erro ao obter índice da interface");
        close(rsocket);
        return -2;
    }

    // Configurar o endereço do socket
    memset(&local, 0, sizeof(local));
    local.sll_family = AF_PACKET;
    local.sll_ifindex = ifr.ifr_ifindex;
    local.sll_protocol = htons(ETH_P_ALL);

    // Bind do socket à interface
    if (bind(rsocket, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("Erro ao fazer bind do socket à interface");
        close(rsocket);
        return -3;
    }

    // Configurar o modo promíscuo
    memset(&mreq, 0, sizeof(mreq));
    mreq.mr_ifindex = ifr.ifr_ifindex;
    mreq.mr_type = PACKET_MR_PROMISC;

    if (setsockopt(rsocket, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Erro ao configurar modo promíscuo");
        close(rsocket);
        return -4;
    }

    return rsocket;
}


// Função para criar um socket raw
int cria_raw_socket(char *nome_da_interface_rede) 
{
    // Cria um socket raw
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        perror("Erro ao criar socket");
        return -1;
    }

    // Obtém o índice da interface de rede
    struct ifreq ir;
    memset(&ir, 0, sizeof(struct ifreq));
    strncpy(ir.ifr_name, nome_da_interface_rede, IFNAMSIZ - 1);

    // Obter o índice da interface de rede
    if (ioctl(soquete, SIOCGIFINDEX, &ir) == -1) {
        perror("Erro ao obter índice da interface");
        close(soquete);
        return -1;
    }

    // Associa o socket à interface de rede
    struct sockaddr_ll endereco;
    memset(&endereco, 0, sizeof(endereco));
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ir.ifr_ifindex;

    // Inicializa o socket
    if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1) {
        perror("Erro ao fazer bind no socket");
        close(soquete);
        return -1;
    }

    // Configura o modo promíscuo
    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ir.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;

    // Configura o socket para o modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        perror("Erro ao configurar setsockopt");
        close(soquete);
        return -1;
    }

    return soquete;
}
