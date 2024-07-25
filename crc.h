#ifndef CRC_H
#define CRC_H
#include "protocolo.h"

// Função para calcular o CRC-8
uint8_t calc_crc8_with_table(uint8_t *msg, int tamBytes);

// Função para calcular o CRC-8
uint8_t calc_crc8(protocolo *msg);

// Função para calcular o CRC-8
uint8_t gencrc(uint8_t *data, size_t len);

#endif  // CRC_H