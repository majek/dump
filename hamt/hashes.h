#include <sys/types.h>


u_int32_t haproxy_uri_hash(unsigned char *buf, int len);
u_int32_t sax_hash(unsigned char *buf, int len);
u_int32_t hashpjw(unsigned char *buf, int len);
u_int32_t hash_djbx33(unsigned char *buf, int len);
u_int32_t bernstein(unsigned char *buf, int len);
u_int32_t SuperFastHash(unsigned char *buf, int len);
u_int32_t SuperFastHash2(unsigned char *buf, int len);
u_int32_t fnv_32a_str(unsigned char *buf, int len);
u_int32_t hashword(unsigned char *buf, int len);
u_int32_t kr_hash(unsigned char *buf, int len);
u_int32_t fnv_hash(unsigned char *buf, int len);
u_int32_t oat_hash(unsigned char *buf, int len);
u_int32_t wt_hash(unsigned char *buf, int len);
u_int32_t sha1(unsigned char *buf, int len);


/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT4 defines a four byte word */
typedef unsigned long int UINT4;

/* BYTE defines a unsigned character */
typedef unsigned char BYTE;

typedef struct 
{
	UINT4 digest[ 5 ];            /* Message digest */
	UINT4 countLo, countHi;       /* 64-bit bit count */
	UINT4 data[ 16 ];             /* SHS data buffer */
	int Endianness;
} SHA_CTX;

/* Message digest functions */

void SHAInit(SHA_CTX *);
void SHAUpdate(SHA_CTX *, BYTE *buffer, int count);
void SHAFinal(BYTE *output, SHA_CTX *);

