#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

typedef uint8_t u8;
typedef uint32_t u32;

struct key128_ctx {
	u8 key[11][16];
};

void expand_key128(struct key128_ctx *ctx, u8 *key);
void ctr_stream(struct key128_ctx *ctx, u8 *out, u32 len, u8 *iv);


static int raw_from_c(int a) {
	switch(a) {
	case '0'...'9':
		return a - '0';
	case 'a'...'f':
		return a - 'a' + 10;
	default:
		abort();
	}
	return -1;
}

static u8 *hex(char *in, int *len_ptr) {
	int i=0;
	int len = strlen(in)/2;
	u8 *out = malloc(len + 1);
	while(*in) {
		int a = raw_from_c(*in++);
		int b = raw_from_c(*in++);
		out[i++] = a * 16 + b;
	}
	if (len_ptr)
		*len_ptr = len;
	return out;
}

static void xor(u8 *dst, u8 *op, u32 len) {
	while (len) {
		*dst = *dst ^ *op;
		dst ++;
		op ++;
		len --;	
	}
}

int main() {
    u8 *key = hex("36f18357be4dbd77f050515c73fcf9f2", NULL);
    u8 *iv  = hex("69dda8455c7dd4254bf353b773304eec", NULL);

    struct key128_ctx ctx;
    expand_key128(&ctx, key);

    u8 out[512];
    ctr_stream(&ctx, out, sizeof(out), iv);

    int msg_len = 0;
    u8 *msg = hex("0ec7702330098ce7f7520d1cbbb20fc388d1b0adb5054dbd7370849dbf0b88d393f252e764f1f5f7ad97ef79d59ce29f5f51eeca32eabedd9afa9329", &msg_len);

    xor(out, msg, msg_len);
    out[msg_len] = '\0';

    printf("out=%s\n", out);
    return 0;
}
