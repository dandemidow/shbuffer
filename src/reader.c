#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "sharedmem/shalloc.h"

typedef struct t_t {
  char name[20];
  int number;
  int second;
  struct t_t *next;
} test_t;

typedef struct {
  test_t *read;
  test_t *write;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int active;
} pre_t;

int main () {
  printf("shared memory read\n");
  shared_mem_t *mem = init_link_shared_mem(1024, "my_buf");

  int i;
  pre_t *fbl = (pre_t *)get_first_block_mem(mem);
  for ( i=0; i<100; i++ ) {
    pthread_mutex_lock(&fbl->mutex);
    while( !fbl->read && fbl->active )
      pthread_cond_wait(&fbl->cond, &fbl->mutex);
    if ( !fbl->read ) {
      printf("lock fail!\n");
      break;
    }
    test_t *ptr = fix_ptr_to(test_t, mem, fbl->read);
    fbl->read = ptr->next;
    printf("test string %s, %d, %d\n", ptr->name, ptr->number, ptr->second);
    free_shared_mem(mem, ptr);
    pthread_mutex_unlock(&fbl->mutex);
  }

  close_link_shared_mem(mem);
  return 0;
}
