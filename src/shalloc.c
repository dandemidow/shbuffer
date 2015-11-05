#include "sharedmem/shalloc.h"

#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

static shared_mem_t *work_buf;

typedef struct {
  char *base;
  int clients;
  pthread_mutex_t protect;
} shared_stuff_t;

#define shared_stuff(shm) ((shared_stuff_t*)(shm)->addr)
#define shared_client(shm) ((shared_stuff(shm))->clients)
#define shared_protect(shm) (&shared_stuff(shm)->protect)

static shared_mem_block_t *get_first_block() {
  return (shared_mem_block_t*)(work_buf->addr + sizeof(shared_stuff_t));
}

void *fix_pointer(void *ptr) {
  return work_buf->addr + ((char*)ptr - work_buf->base);
}

int init_shared_mem(size_t buf_size, char *name) {
  work_buf = malloc(sizeof(shared_mem_t));
  init_shared_buffer(work_buf, buf_size, name);
  work_buf->base = work_buf->addr;

  pthread_mutexattr_t mta;
  pthread_mutexattr_init(&mta);
  pthread_mutexattr_setpshared(&mta, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(shared_protect(work_buf), &mta);

  pthread_mutex_lock(shared_protect(work_buf));
  shared_stuff(work_buf)->base = work_buf->base;
  shared_client(work_buf) = 0;
  shared_mem_block_t *first = get_first_block();
  first->availabel = 1;
  first->size = work_buf->buf_size - sizeof(shared_mem_block_t);
  pthread_mutex_unlock(shared_protect(work_buf));
  return 0;
}

int init_link_shared_mem(size_t buf_size, char *name) {
  work_buf = malloc(sizeof(shared_mem_t));
  init_link_shared_buffer(work_buf, buf_size, name);
  pthread_mutex_lock(shared_protect(work_buf));
  shared_client(work_buf) +=1;
  work_buf->base = shared_stuff(work_buf)->base;
  pthread_mutex_unlock(shared_protect(work_buf));
  return 0;
}

int close_shared_mem() {
  close_shared_buffer(work_buf);
  free(work_buf);
  return 0;
}

int close_link_shared_mem() {
  shared_client(work_buf) -=1;
  close_link_shared_buffer(work_buf);
  free(work_buf);
  return 0;
}

static void insert_block(shared_mem_block_t *block, int size) {
  short old_size = block->size;
  block->size = size;
  block->availabel = 0;
  char *ptr = (char*)block;
  ptr+=sizeof(shared_mem_block_t) + size;
  shared_mem_block_t *nblk = (shared_mem_block_t *)ptr;
  nblk->availabel = 1;
  nblk->size = old_size - sizeof(shared_mem_block_t) - size;
}

void *alloc_shared_mem(size_t size) {
  shared_mem_block_t *block = get_first_block();
  char *ptr = (char *)block;
  pthread_mutex_lock(shared_protect(work_buf));
  while ( (size_t)(ptr - work_buf->addr) < work_buf->buf_size ) {
    if ( block->availabel && size <= block->size ) {
      insert_block(block, size);
      pthread_mutex_unlock(shared_protect(work_buf));
      return ptr + sizeof(shared_mem_block_t);
    }
    ptr += sizeof(shared_mem_block_t) + block->size;
    block = (shared_mem_block_t *)ptr;
  }
  pthread_mutex_unlock(shared_protect(work_buf));
  return NULL;
}

int free_shared_mem(void *mem) {
  pthread_mutex_lock(shared_protect(work_buf));
  shared_mem_block_t *nblk = (shared_mem_block_t *)(mem-sizeof(shared_mem_block_t));
  nblk->availabel = 1;
  pthread_mutex_unlock(shared_protect(work_buf));
  return 0;
}

void *get_first_block_mem() {
  return (void*)((char *)get_first_block() + sizeof(shared_mem_block_t));
}

void wait_shared_client_exit() {
  int clients;
  pthread_mutex_lock(shared_protect(work_buf));
  clients = shared_client(work_buf);
  pthread_mutex_unlock(shared_protect(work_buf));
  while( clients != 0) {
    sem_wait(work_buf->exit_sem);
    pthread_mutex_lock(shared_protect(work_buf));
    clients = shared_client(work_buf);
    pthread_mutex_unlock(shared_protect(work_buf));
  }
}
