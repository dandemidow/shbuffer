#ifndef _SHAREDMEM_BUFFER_H_
#define _SHAREDMEM_BUFFER_H_

#include <semaphore.h>

#define BUF_SUCCESS 0
#define BUF_MMAP_ERR -1
#define BUF_INIT_ISEM_ERR -2
#define BUF_INIT_ESEM_ERR -3
#define BUF_SHM_OPEN_ERR -4
#define BUF_SHM_TRUNC_ERR -5
#define BUF_EXIT_ISEM_ERR -6
#define BUF_EXIT_ESEM_ERR -7
#define BUF_SHM_CLOSE_ERR -8
#define BUF_UNMMAP_ERR -9

typedef struct {
  int shm_fd;
  char *addr;
  char *base;
  sem_t *exit_sem;
  sem_t *init_sem;
  size_t buf_size;
  char *buf_name;
  char *exit_sem_name;
  char *init_sem_name;
} shared_mem_t;

int init_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name);
int init_link_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name);

void *shared_buffer(shared_mem_t *shm);

int close_shared_buffer(shared_mem_t *shm);
int close_link_shared_buffer(shared_mem_t *shm);


#endif  // _SHAREDMEM_BUFFER_H_
