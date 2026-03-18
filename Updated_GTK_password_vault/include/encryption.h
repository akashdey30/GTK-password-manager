#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stddef.h>

#define XOR_KEY 5

void xor_buffer(unsigned char *buf, size_t len);

#endif // ENCRYPTION_H
