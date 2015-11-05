#ifndef _SHAREDMEM_ALLOC_H_
#define _SHAREDMEM_ALLOC_H_

#include "sharedmem/buffer.h"

typedef struct {
  short availabel;
  unsigned size;
} shared_mem_block_t;

void *fix_pointer(void *ptr);

int init_shared_mem(size_t buf_size, char *name);
int init_link_shared_mem(size_t buf_size, char *name);

void wait_shared_client_exit();
int close_shared_mem();
int close_link_shared_mem();

void *alloc_shared_mem(size_t);
int free_shared_mem(void *);

void *get_first_block_mem();

#endif  // _SHAREDMEM_ALLOC_H_
