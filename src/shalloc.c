#include "sharedmem/shalloc.h"

#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct {
  char *base;
  int clients;
  pthread_mutex_t protect;
} shared_stuff_t;

#define shared_stuff(shm) ((shared_stuff_t*)(shm)->addr)
#define shared_client(shm) ((shared_stuff(shm))->clients)
#define shared_protect(shm) (&shared_stuff(shm)->protect)

static shared_mem_block_t *get_first_block(shared_mem_t *shbuf) {
  return (shared_mem_block_t*)(shbuf->addr + sizeof(shared_stuff_t));
}

shared_mem_t *init_shared_mem(size_t buf_size, char *name) {
  shared_mem_t *shbuf = malloc(sizeof(shared_mem_t));
  init_shared_buffer(shbuf, buf_size, name);
  shbuf->base = shbuf->addr;

  pthread_mutexattr_t mta;
  pthread_mutexattr_init(&mta);
  pthread_mutexattr_setpshared(&mta, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(shared_protect(shbuf), &mta);

  pthread_mutex_lock(shared_protect(shbuf));
  shared_stuff(shbuf)->base = shbuf->base;
  shared_client(shbuf) = 0;
  shared_mem_block_t *first = get_first_block(shbuf);
  first->availabel = 1;
  first->size = shbuf->buf_size - sizeof(shared_mem_block_t);
  pthread_mutex_unlock(shared_protect(shbuf));
  return shbuf;
}

shared_mem_t *init_link_shared_mem(size_t buf_size, char *name) {
  shared_mem_t *shbuf = malloc(sizeof(shared_mem_t));
  init_link_shared_buffer(shbuf, buf_size, name);
  pthread_mutex_lock(shared_protect(shbuf));
  shared_client(shbuf) +=1;
  shbuf->base = shared_stuff(shbuf)->base;
  pthread_mutex_unlock(shared_protect(shbuf));
  return shbuf;
}

int close_shared_mem(shared_mem_t *shbuf) {
  int r = close_shared_buffer(shbuf);
  free(shbuf);
  return r;
}

int close_link_shared_mem(shared_mem_t *shbuf) {
  int r;
  shared_client(shbuf) -=1;
  r = close_link_shared_buffer(shbuf);
  free(shbuf);
  return r;
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

void *alloc_shared_mem(shared_mem_t *shbuf, size_t size) {
  shared_mem_block_t *block = get_first_block(shbuf);
  char *ptr = (char *)block;
  pthread_mutex_lock(shared_protect(shbuf));
  while ( (size_t)(ptr - shbuf->addr) < shbuf->buf_size ) {
    if ( block->availabel && size <= block->size ) {
      insert_block(block, size);
      pthread_mutex_unlock(shared_protect(shbuf));
      return ptr + sizeof(shared_mem_block_t);
    }
    ptr += sizeof(shared_mem_block_t) + block->size;
    block = (shared_mem_block_t *)ptr;
  }
  pthread_mutex_unlock(shared_protect(shbuf));
  return NULL;
}

int free_shared_mem(shared_mem_t *shbuf, void *mem) {
  pthread_mutex_lock(shared_protect(shbuf));
  shared_mem_block_t *nblk = (shared_mem_block_t *)(mem-sizeof(shared_mem_block_t));
  nblk->availabel = 1;
  pthread_mutex_unlock(shared_protect(shbuf));
  return 0;
}

void *get_first_block_mem(shared_mem_t *shbuf) {
  return (void*)((char *)get_first_block(shbuf) + sizeof(shared_mem_block_t));
}

void wait_shared_client_exit(shared_mem_t *shbuf) {
  int clients;
  pthread_mutex_lock(shared_protect(shbuf));
  clients = shared_client(shbuf);
  pthread_mutex_unlock(shared_protect(shbuf));
  while( clients != 0) {
    sem_wait(shbuf->exit_sem);
    pthread_mutex_lock(shared_protect(shbuf));
    clients = shared_client(shbuf);
    pthread_mutex_unlock(shared_protect(shbuf));
  }
}
