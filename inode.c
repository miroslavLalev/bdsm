#include <stdint.h>

// octal numbers for inode type
const uint8_t M_DIR = 01;
const uint8_t M_FILE = 02;
const uint8_t M_SLNK = 04;

// octal numbers for permission bits
const uint8_t M_EXEC = 01;
const uint8_t M_WRITE = 02;
const uint8_t M_READ = 04;
