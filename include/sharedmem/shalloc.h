#ifndef _SHAREDMEM_ALLOC_H_
#define _SHAREDMEM_ALLOC_H_

#include <stdio.h>

#include "sharedmem/buffer.h"

#define MEM_NULL_PTR -100

typedef struct {
  unsigned char available;
  unsigned char tag;
  unsigned size;
} shared_mem_block_t;

inline char *loc_cast_char(const shared_mem_t *const shbuf, void *ptr);
inline char *glob_cast_char(const shared_mem_t *const shbuf, void *ptr);
#define locl_cast(type, shbuf, ptr) ((type*)(loc_cast_char(shbuf, ptr)))
#define glob_cast(type, shbuf, ptr) ((type*)(glob_cast_char(shbuf, ptr)))

void shared_mutex_init(pthread_mutex_t *);
void shared_cond_init(pthread_cond_t *);

shared_mem_t *init_shared_mem(size_t buf_size, const char *name, int *state);
shared_mem_t *init_link_shared_mem(size_t buf_size, const char *name, int *state);

void wait_shared_client_init(const shared_mem_t *const);
void wait_shared_client_exit(const shared_mem_t *const);
int close_shared_mem(shared_mem_t *);
int close_link_shared_mem(shared_mem_t *);

void *alloc_shared_mem(const shared_mem_t *const, size_t);
int free_shared_mem(const shared_mem_t *const, void *);

void tag_shared_mem(const shared_mem_t *const, void *, unsigned char tag);
void *find_tagged_mem(const shared_mem_t *const, unsigned char tag);

int clients_shared_mem(const shared_mem_t *const);
pid_t master_pid(const shared_mem_t *const shbuf);

#endif  // _SHAREDMEM_ALLOC_H_
