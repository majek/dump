#if __GNUC__
#  define likely(e) __builtin_expect((e), 1)
#  define unlikely(e) __builtin_expect((e), 0)
#  define prefetch(r) __builtin_prefetch(r)
#  define PACKED __attribute__ ((packed))
#else
#  warning "Please set compiler specific macros!"
#  define likely(e) (e)
#  define unlikely(e) (e)
#  define prefetch(r) (r)
#  define PACKED __attribute__ ((packed))
#endif


#ifndef offsetof
#define offsetof(st, m) __builtin_offsetof(st, m)
//#  define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(ptr, type, member) ((type *)((char *)ptr - offsetof(type, member)))

//#  define container_of(ptr, type, member) ({
//                      const typeof( ((type *)0)->member ) *__mptr = (ptr);
//                      (type *)( (char *)__mptr - offsetof(type,member) );})
#endif
