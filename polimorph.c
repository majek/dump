#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#define MAGIC 0xDEADBABE

struct record {
	int magic;
	struct subrecord_a {
		int a;
	} subrecord_a;
	struct subrecord_b {
		int b;
	} subrecord_b;
} record;

#define info(format, ...)	gl_log("INFO", format, ##__VA_ARGS__)
#define warn(format, ...)	gl_log("WARN", format, ##__VA_ARGS__)
#define error(format, ...)	gl_log("ERROR", format, ##__VA_ARGS__)

void gl_log(const char *type, const char *s, ...) {
	char buf[1024];
	va_list p;
	va_start(p, s);
	if(s == 0) s = "";
	vsnprintf(buf, sizeof(buf), s, p);
	va_end(p);
	fflush(stdout);
	fprintf(stderr, " [*] %8s  %s\n", type, buf);
}

// Copy from linux kernel 2.6 source (kernel.h, stddef.h)
#define container_of(ptr, type, member) ({      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
	(type *)( (char *)__mptr - offsetof(type,member) );})
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define	rb_entry(ptr, type, member) container_of(ptr, type, member)

//struct item *item = container_of(node, struct item, node);

//t __builtin_types_compatible_p
#undef __CONCAT
#define __CONCAT1(x,y)  x ## y
#define __CONCAT(x,y)   __CONCAT1(x,y)


a = {foo_int, };

#define foo(a)({					\
	if(__builtin_types_compatible_p(typeof(a),
	foo2(a, typeof(a) )

#define foo2(a, x)	\
	__CONCAT(foo_, __CONCAT(x,x))(a)


void foo_int(int i) {
	printf("int %s\n", );
}

int main() {
	foo(1);
	return(0);
}









