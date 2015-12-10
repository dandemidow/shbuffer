#include <stdio.h>
#include <string.h>
#include <unistd.h>
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
  printf("shared memory test\n");
  shared_mem_t *mem = init_shared_mem(1024, "my_buf", NULL);

  int i=0;
  pre_t *pre = (pre_t*)alloc_shared_mem(mem, sizeof(pre_t));
  tag_shared_mem(mem, (void*)pre, 1);
  pre->read = NULL;
  pre->write = NULL;
  shared_mutex_init(&pre->mutex);
  shared_cond_init(&pre->cond);
  pre->active = 1;
  test_t *last;

  for ( i =0; i<100; ++i) {
    pthread_mutex_lock(&pre->mutex);
    test_t *tst = (test_t*)alloc_shared_mem(mem, sizeof(test_t));
    if ( !tst ) {
      printf("shalloc error\n");
      break;
    }
    strcpy(tst->name, "hello!");
    tst->number = 0+i;
    tst->second = 100-i;
    tst->next = NULL;
    if ( !pre->read ) {
      pre->read = tst;
      last = pre->read;
    }
    else {
      last->next = tst;
      last = last->next;
    }
    pthread_cond_signal(&pre->cond);
    pthread_mutex_unlock(&pre->mutex);
    printf("add value %d\n", i);
    sleep(1);
  }
  pre->active = 0;
  wait_shared_client_exit(mem);
  close_shared_mem(mem);
  return 0;
}
