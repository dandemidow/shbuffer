#ifndef _SHAREDMEM_ALLOC_H_
#define _SHAREDMEM_ALLOC_H_

#include "sharedmem/buffer.h"

typedef struct {
  unsigned char available;
  unsigned char tag;
  unsigned size;
} shared_mem_block_t;

#define fix_ptr(shbuf, ptr) ((shbuf)->addr + ((char*)(ptr) - (shbuf)->base))
#define fix_ptr_to(type, shbuf, ptr) (type*)((shbuf)->addr + ((char*)(ptr) - (shbuf)->base))

shared_mem_t *init_shared_mem(size_t buf_size, char *name);
shared_mem_t *init_link_shared_mem(size_t buf_size, char *name);

void wait_shared_client_exit(shared_mem_t *);
int close_shared_mem(shared_mem_t *);
int close_link_shared_mem(shared_mem_t *);

void *alloc_shared_mem(shared_mem_t *, size_t);
int free_shared_mem(shared_mem_t *, void *);

void tag_shared_mem(shared_mem_t *, void *, unsigned char tag);
void *find_tagged_mem(shared_mem_t *, unsigned char tag);

#endif  // _SHAREDMEM_ALLOC_H_
