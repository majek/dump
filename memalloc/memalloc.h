#ifndef _MEMALLOC
#define _MEMALLOC

struct mem_context;

struct mem_context *mem_context_new();
void mem_context_free(struct mem_context *mc);
unsigned long mem_context_allocated(struct mem_context *mc);

void *mem_alloc(struct mem_context *mc, unsigned size);
void mem_free(struct mem_context *mc, void *ptr, unsigned size);




#endif // _MEMALLOC
