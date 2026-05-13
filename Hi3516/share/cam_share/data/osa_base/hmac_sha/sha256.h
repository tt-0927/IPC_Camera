#ifndef SHA256_H
#define SHA256_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define SHA256_BLOCK_SIZE 32            // SHA256 outputs a 32 byte digest

/**************************** DATA TYPES ****************************/
typedef unsigned char BYTE;             // 8-bit byte
typedef unsigned int  WORD;             // 32-bit word, change to "long" for 16-bit machines

typedef struct {
	BYTE data[64];  // current 512-bit chunk of message data, just like a buffer
	WORD datalen;   // sign the data length of current chunk
	unsigned long long bitlen;  // the bit length of the total message
	WORD state[8];  // store the middle state of hash abstract
} SHA256_CTX;

/*********************** FUNCTION DECLARATIONS **********************/
void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const BYTE data[], size_t len);
void sha256_final(SHA256_CTX *ctx, BYTE hash[]);

int sha256(const unsigned char *input, int inputSize,unsigned char output[SHA256_BLOCK_SIZE]);

void hmac_sha256(const unsigned char *key, unsigned int key_len,const unsigned char *data, \
		unsigned int data_len,unsigned char output[SHA256_BLOCK_SIZE]);


#endif   // SHA256_H
