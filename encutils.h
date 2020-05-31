#ifndef ENCUTILS_H
#define ENCUTILS_H

#include <stdlib.h>
#include <stdint.h>

void enc_u16(uint16_t val, uint8_t *data, size_t *offset);
uint16_t dec_u16(uint8_t *data, size_t *offset);

void enc_u32(uint32_t val, uint8_t *data, size_t *offset);
uint32_t dec_u32(uint8_t *data, size_t *offset);

void enc_u64(uint64_t val, uint8_t *data, size_t *offset);
uint64_t dec_u64(uint8_t *data, size_t *offset);

void enc_str(char *str, size_t size, uint8_t *data, size_t *offset);
char *dec_str(uint8_t *data, size_t *offset, size_t size);

#endif
