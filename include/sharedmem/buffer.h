#ifndef _SHAREDMEM_BUFFER_H_
#define _SHAREDMEM_BUFFER_H_

#include <semaphore.h>

typedef struct {
  int shm_fd;
  char *addr;
  char *base;
  sem_t *exit_sem;
  size_t buf_size;
  char *buf_name;
  char *exit_sem_name;
} shared_mem_t;

int init_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name);
int init_link_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name);

void *shared_buffer(shared_mem_t *shm);

int close_shared_buffer(shared_mem_t *shm);
int close_link_shared_buffer(shared_mem_t *shm);


#endif  // _SHAREDMEM_BUFFER_H_
