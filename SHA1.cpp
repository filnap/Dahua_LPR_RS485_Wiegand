#include "SHA1.h"

#define ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static void SHA1Transform(uint32_t state[5], const uint8_t buffer[64]) {
  uint32_t a, b, c, d, e;
  typedef union {
    uint8_t c[64];
    uint32_t l[16];
  } CHAR64LONG16;

  CHAR64LONG16 block;
  memcpy(&block, buffer, 64);

  uint32_t w[80];
  for (int i = 0; i < 16; i++) w[i] = __builtin_bswap32(block.l[i]);
  for (int i = 16; i < 80; i++)
    w[i] = ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

  a = state[0];
  b = state[1];
  c = state[2];
  d = state[3];
  e = state[4];

  for (int i = 0; i < 80; i++) {
    uint32_t f, k;
    if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999; }
    else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
    else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
    else { f = b ^ c ^ d; k = 0xCA62C1D6; }

    uint32_t temp = ROL(a, 5) + f + e + k + w[i];
    e = d;
    d = c;
    c = ROL(b, 30);
    b = a;
    a = temp;
  }

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
}

void SHA1Init(SHA1_CTX* context) {
  context->state[0] = 0x67452301;
  context->state[1] = 0xEFCDAB89;
  context->state[2] = 0x98BADCFE;
  context->state[3] = 0x10325476;
  context->state[4] = 0xC3D2E1F0;
  context->count[0] = context->count[1] = 0;
}

void SHA1Update(SHA1_CTX* context, const uint8_t* data, uint32_t len) {
  uint32_t i, j;
  j = (context->count[0] >> 3) & 63;
  if ((context->count[0] += len << 3) < (len << 3)) context->count[1]++;
  context->count[1] += (len >> 29);

  if ((j + len) > 63) {
    memcpy(&context->buffer[j], data, (i = 64 - j));
    SHA1Transform(context->state, context->buffer);
    for (; i + 63 < len; i += 64)
      SHA1Transform(context->state, &data[i]);
    j = 0;
  } else i = 0;

  memcpy(&context->buffer[j], &data[i], len - i);
}

void SHA1Final(uint8_t digest[20], SHA1_CTX* context) {
  uint8_t finalcount[8];

  for (int i = 0; i < 8; i++) {
    finalcount[i] = (uint8_t)((context->count[(i >= 4 ? 0 : 1)]
      >> ((3 - (i & 3)) * 8)) & 255);
  }

  SHA1Update(context, (uint8_t*)"\200", 1);
  while ((context->count[0] & 504) != 448)
    SHA1Update(context, (uint8_t*)"\0", 1);

  SHA1Update(context, finalcount, 8);

  for (int i = 0; i < 20; i++) {
    digest[i] = (uint8_t)
      ((context->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
  }
}
