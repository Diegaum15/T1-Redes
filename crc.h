#ifndef CRC_H
#define CRC_H
#include "protocolo.h"

// Função para calcular o CRC-8
uint8_t calc_crc8_with_table(uint8_t *msg, int tamBytes);

// Função para calcular o CRC-8
uint8_t calc_crc8(protocolo *msg);

#endif  // CRC_H