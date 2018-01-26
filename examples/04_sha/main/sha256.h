#pragma once

#define uchar unsigned char // 8-bit byte
#define uint unsigned int // 32-bit word
#include <string.h>

typedef struct {
   uchar data[64];
   uint datalen;
   uint bitlen[2];
   uint state[8];
} SHA256_CTX;

void tsha256_init(SHA256_CTX *ctx);
void tsha256_update(SHA256_CTX *ctx, const uchar *data, uint len);
void tsha256_final(SHA256_CTX *ctx, uchar hash[]);