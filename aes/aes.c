#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

static void *malloc_aligned(unsigned int size)
{
	void *ptr;
	int r = posix_memalign(&ptr, 256, size);
	if (r != 0) {
		perror("malloc error");
		abort();
	}
	memset(ptr, 0, size);
	return ptr;
}


#include <openssl/aes.h>

typedef struct {
	unsigned char v[16];
} block_t;

char map[16] = {'0','1','2','3','4', '5', '6','7','8','9','a','b','c','d','e','f'};
char *tohex(block_t in) {
	static char buf[64];
	int i;
	for(i=0; i < 16; i++) {
		unsigned char c = (unsigned char)in.v[i];
		buf[i*2] = map[c >> 4];
		buf[i*2 + 1] = map[c & 0x0f];
	}
	buf[i*2] = 0;
	return buf;
}

block_t encrypt128(block_t in, block_t in_key) {
	block_t out;
        AES_KEY key;
	memset(&key, 0, sizeof(AES_KEY));
 
	AES_set_encrypt_key((unsigned char*)&in_key.v, 128, &key);
	AES_ecb_encrypt((unsigned char*)&in.v, (unsigned char*)&out.v, &key, AES_ENCRYPT);
	return out;
}


#define AES_KEYSIZE_128         16
#define AES_MAX_KEYLENGTH       (15 * 16)
#define AES_MAX_KEYLENGTH_U32   (AES_MAX_KEYLENGTH / sizeof(uint32_t))

struct crypto_aes_ctx {
	uint32_t key_enc[AES_MAX_KEYLENGTH_U32];
	uint32_t key_dec[AES_MAX_KEYLENGTH_U32];
	uint32_t key_length;
};

int aesni_set_key(struct crypto_aes_ctx *ctx, const uint8_t *in_key, unsigned int key_len);
void aesni_ecb_enc(struct crypto_aes_ctx *ctx, const uint8_t *dst, uint8_t *src, size_t len);


struct crypto_aes_ctx *ctx;// = malloc_aligned(sizeof(struct crypto_aes_ctx));

block_t encrypt128_intel(block_t in, block_t in_key) {
	block_t out;
	aesni_set_key(ctx, &in_key.v, AES_KEYSIZE_128);
	aesni_ecb_enc(ctx, &out.v, &in.v, 16);
	return out;
}


int main() {
	ctx = malloc_aligned(sizeof(struct crypto_aes_ctx));

	block_t in = {.v = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}};
	block_t key = {.v = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}};
	block_t out;

	int i;
	for (i=0; i< 200000000; i++) {
		key.v[15] = i % 256;
		out = encrypt128(in, key);
//		out = encrypt128_intel(in, key);
	}
	return;
	out = encrypt128(in, key);

	printf("in  = %s\n", tohex(in));
	printf("key = %s\n", tohex(key));
	printf("out = %s\n", tohex(out));
	printf("\n");
	
	out = encrypt128_intel(in, key);
	printf("in  = %s\n", tohex(in));
	printf("key = %s\n", tohex(key));
	printf("out = %s\n", tohex(out));
	return 0;
}
