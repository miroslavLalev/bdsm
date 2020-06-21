#include <stdlib.h>
#include <stdint.h>
#include <string.h>

void enc_u16(uint16_t val, uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    data[ov] = (val >> 8) & 0xFF;
    data[ov+1] = val & 0xFF;
    *offset+=2;
}

uint16_t dec_u16(uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    uint16_t res = data[ov+1] | ((uint16_t)data[ov] << 8);
    *offset+=2;
    return res;
}

void enc_u32(uint32_t val, uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    data[ov] = (val >> 24) & 0xFF;
    data[ov+1] = (val >> 16) & 0xFF;
    data[ov+2] = (val >> 8) & 0xFF;
    data[ov+3] = val & 0xFF;
    *offset+=4;
}

uint32_t dec_u32(uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    uint32_t res = data[ov+3] |
                ((uint16_t)data[ov+2] << 8) |
                ((uint32_t)data[ov+1] << 16) |
                ((uint32_t)data[ov] << 24);
    *offset+=4;
    return res;
}

void enc_u64(uint64_t val, uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    data[ov] = (val >> 56) & 0xFF;
    data[ov+1] = (val >> 48) & 0xFF;
    data[ov+2] = (val >> 40) & 0xFF;
    data[ov+3] = (val >> 32) & 0xFF;
    data[ov+4] = (val >> 24) & 0xFF;
    data[ov+5] = (val >> 16) & 0xFF;
    data[ov+6] = (val >> 8) & 0xFF;
    data[ov+7] = val & 0xFF;
    *offset+=8;
}

uint64_t dec_u64(uint8_t *data, size_t *offset) {
    size_t ov = *offset;
    uint64_t res = data[ov+7] |
                ((uint16_t)data[ov+6] << 8) |
                ((uint32_t)data[ov+5] << 16) |
                ((uint32_t)data[ov+4] << 24) |
                ((uint64_t)data[ov+3] << 32) |
                ((uint64_t)data[ov+2] << 40) |
                ((uint64_t)data[ov+1] << 48) |
                ((uint64_t)data[ov] << 56);
    *offset+=8;
    return res;
}

void enc_str(char *str, size_t size, uint8_t *data, size_t *offset) {
    size_t i;
    for (i=0; i<size; i++) {
        data[*offset+i] = str[i]; // 1b - 1b
    }
    *offset+=size;
}

void dec_str(uint8_t *data, size_t *offset, char *res, size_t size) {
    size_t i;
    for (i=0; i<size; i++) {
        res[i] = data[*offset+i];
    }
    *offset+=size;
}
