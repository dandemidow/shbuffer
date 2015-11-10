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

#define shared_block_for(mem) (shared_mem_block_t *)((mem)-sizeof(shared_mem_block_t))
#define set_block_tag(block, val) (block)->tag = (val)
#define set_block_size(block, s) (block)->size = (s)
#define available_block_on(block) (block)->available = 1;
#define available_block_off(block) (block)->available = 0;

static shared_mem_block_t *get_first_block(shared_mem_t *shbuf) {
  return (shared_mem_block_t*)(shbuf->addr + sizeof(shared_stuff_t));
}

static shared_mem_block_t *next_block(shared_mem_t *shbuf, shared_mem_block_t *block) {
  char *ptr = (char*)block;
  ptr += sizeof(shared_mem_block_t) + block->size;
  if ( !shbuf || (size_t)((char*)block - shbuf->addr) < shbuf->buf_size - block->size )
    return (shared_mem_block_t*)ptr;
  return NULL;
}

static shared_mem_block_t *prev_block(shared_mem_t *shbuf, shared_mem_block_t *block) {
  shared_mem_block_t *base = get_first_block(shbuf);
  shared_mem_block_t *next = base;
  if ( base == block ) return NULL;
  while(next) {
    next = next_block(shbuf, base);
    if ( next == block ) return base;
    base = next;
  }
  return NULL;
}

// log log log
static void log_shared_block(shared_mem_t *shbuf) {
  shared_mem_block_t *block = get_first_block(shbuf);
  printf("------- %d\n", shbuf->buf_size);
  while ( block ) {
    printf("block place:%d, available:%d, size:%d\n",
           (size_t)((char*)block - shbuf->addr),
           block->available,
           block->size);
    block = next_block(shbuf, block);
  }
  printf("-------\n");
}

void shared_mutex_init(pthread_mutex_t *mtx) {
  pthread_mutexattr_t mta;
  pthread_mutexattr_init(&mta);
  pthread_mutexattr_setpshared(&mta, PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(mtx, &mta);
}

void shared_cond_init(pthread_cond_t *cond) {
  pthread_condattr_t cta;
  pthread_condattr_init(&cta);
  pthread_condattr_setpshared(&cta, PTHREAD_PROCESS_SHARED);
  pthread_cond_init(cond, &cta);
}

shared_mem_t *init_shared_mem(size_t buf_size, char *name) {
  shared_mem_t *shbuf = malloc(sizeof(shared_mem_t));
  init_shared_buffer(shbuf, buf_size, name);
  shbuf->base = shbuf->addr;

  shared_mutex_init(shared_protect(shbuf));

  pthread_mutex_lock(shared_protect(shbuf));
  shared_stuff(shbuf)->base = shbuf->base;
  shared_client(shbuf) = 0;
  shared_mem_block_t *first = get_first_block(shbuf);
  available_block_on(first);
  set_block_tag(first, 0);
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
  set_block_size(block, size);
  available_block_off(block);
  set_block_tag(block, 0);
  shared_mem_block_t *nblk = next_block(NULL, block);
  available_block_on(nblk);
  set_block_size(nblk, old_size - size - sizeof(shared_mem_block_t));
}

static shared_mem_block_t *find_shared_block(shared_mem_t *shbuf, size_t size) {
  shared_mem_block_t *block = get_first_block(shbuf);
  while ( block ) {
    if ( block->available && block->size >= size )
      return block;
    block = next_block(shbuf, block);
  }
  return NULL;
}

void *alloc_shared_mem(shared_mem_t *shbuf, size_t size) {
  shared_mem_block_t *block;
  pthread_mutex_lock(shared_protect(shbuf));
  block = find_shared_block(shbuf, size);
  if ( block ) {
    insert_block(block, size);
    pthread_mutex_unlock(shared_protect(shbuf));
//    log_shared_block(shbuf);
    return (((void*)block) + sizeof(shared_mem_block_t));
  }
  pthread_mutex_unlock(shared_protect(shbuf));
//  printf("not enough memory\n");
//  log_shared_block(shbuf);
  return NULL;
}

int free_shared_mem(shared_mem_t *shbuf, void *mem) {
  pthread_mutex_lock(shared_protect(shbuf));
  shared_mem_block_t *curr = shared_block_for(mem);
  shared_mem_block_t *prev = prev_block(shbuf, curr);
  shared_mem_block_t *next = next_block(shbuf, curr);
//  printf("prev av:%d, size:%d\n", prev->available, prev->size);
//  printf("next av:%d, size:%d\n", next->available, next->size);
  if (next && next->available)
    set_block_size(curr, curr->size + next->size + sizeof(shared_mem_block_t));
  if (prev && prev->available)
    set_block_size(prev, prev->size + curr->size + sizeof(shared_mem_block_t));
  else
    available_block_on(curr);
//  log_shared_block(shbuf);
  pthread_mutex_unlock(shared_protect(shbuf));
  return 0;
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


void tag_shared_mem(shared_mem_t *shbuf, void *mem, unsigned char tag) {
  shared_mem_block_t *curr;
  pthread_mutex_lock(shared_protect(shbuf));
  curr = shared_block_for(mem);
  set_block_tag(curr, tag);
  pthread_mutex_unlock(shared_protect(shbuf));
}


void *find_tagged_mem(shared_mem_t *shbuf, unsigned char tag) {
  shared_mem_block_t *block = get_first_block(shbuf);
  while ( block ) {
    if ( block->tag == tag ) return (void*)((char*)block + sizeof(shared_mem_block_t));
    block = next_block(shbuf, block);
  }
  return NULL;
}

char *loc_cast_char(shared_mem_t *shbuf, void *ptr) {
  return ptr?(shbuf->addr + ((char*)ptr - shbuf->base)):NULL;
}

char *glob_cast_char(shared_mem_t *shbuf, void *ptr) {
  return ptr?(shbuf->base + ((char*)ptr - shbuf->addr)):NULL;
}
