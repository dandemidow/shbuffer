#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <check.h>

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

void *test_read_write_writer(void *arg) {
  int *ret = malloc(sizeof(int));
  int err = 0;
  shared_mem_t *mem = init_shared_mem(1024, "my_buf", NULL);
  ck_assert_ptr_ne(mem, NULL);

  pre_t *pre = (pre_t*)alloc_shared_mem(mem, sizeof(pre_t));
  ck_assert_ptr_ne(pre, NULL);
  tag_shared_mem(mem, (void*)pre, 1);
  pre->read = NULL;
  pre->write = NULL;
  shared_mutex_init(&pre->mutex);
  shared_cond_init(&pre->cond);
  pre->active = 1;
  test_t *last = NULL;

  for ( *ret=0; *ret<*(int*)(arg); (*ret)++) {
    err = pthread_mutex_lock(&pre->mutex);
    ck_assert_int_eq(err, 0);
    test_t *tst = (test_t*)alloc_shared_mem(mem, sizeof(test_t));
    memset(tst, 0, sizeof(test_t));
    ck_assert_ptr_ne(tst, NULL);
    strcpy(tst->name, "hello!");
    tst->number = 0+*ret;
    tst->second = *(int*)(arg)-*ret;
    tst->next = NULL;
    if ( !pre->read ) {
      pre->read = tst;
      last = pre->read;
    }
    else {
      last->next = tst;
      last = last->next;
    }
    err = pthread_cond_signal(&pre->cond);
    ck_assert_int_eq(err, 0);
    err = pthread_mutex_unlock(&pre->mutex);
    ck_assert_int_eq(err, 0);
    usleep(1000);
  }

  pthread_mutex_lock(&pre->mutex);
  pre->active = 0;
  pthread_cond_signal(&pre->cond);
  pthread_mutex_unlock(&pre->mutex);

  wait_shared_client_exit(mem);
  close_shared_mem(mem);
  pthread_exit(ret);
}

void *test_read_write_reader(void *arg) {
  int *ret = malloc(sizeof(int));
  *ret = 0;
  int err = 0;
  shared_mem_t *mem = init_link_shared_mem(1024, "my_buf", NULL);
  ck_assert_ptr_ne(mem, NULL);

  pre_t *fbl = (pre_t *)find_tagged_mem(mem, 1);
  ck_assert_ptr_ne(fbl, NULL);

  while ( fbl->active ) {
    err = pthread_mutex_lock(&fbl->mutex);
    ck_assert_int_eq(err, 0);

    while( !fbl->read && fbl->active )
      pthread_cond_wait(&fbl->cond, &fbl->mutex);
    if ( !fbl->active ) break;
    else if ( !fbl->read ) {
      *ret = -1;
      break;
    }

    test_t *ptr = locl_cast(test_t, mem, fbl->read);
    ck_assert_ptr_ne(ptr, NULL);

    fbl->read = ptr->next;
    if (0)
      printf("test string %s, %d, %d\n", ptr->name, ptr->number, ptr->second);
    ck_assert_str_eq(ptr->name, "hello!");
    ck_assert_int_eq(ptr->number, *ret);
    ck_assert_int_eq(ptr->second, *(int*)(arg) - *ret);
    (*ret)++;
    err = free_shared_mem(mem, ptr);
    ck_assert_int_eq(err, 0);
    pthread_mutex_unlock(&fbl->mutex);
  }
  close_link_shared_mem(mem);
  pthread_exit(ret);
}

START_TEST(test_read_write)
{
  pthread_t wr, rd;
  int count = 100;
  pthread_create(&wr, NULL, test_read_write_writer, &count);
  usleep(1000);
  pthread_create(&rd, NULL, test_read_write_reader, &count);

  int *rd_count = NULL;
  int *wr_count = NULL;

  pthread_join(rd, (void**)&rd_count);
  ck_assert_int_eq(*rd_count, count);
  free(rd_count);

  pthread_join(wr, (void**)&wr_count);
  ck_assert_int_eq(*wr_count, count);
  free(wr_count);

}
END_TEST

START_TEST(test_master_pid)
{
  int err;
  pid_t pid = getpid();
  ck_assert_int_ne(pid, 0);
  shared_mem_t *mem = init_shared_mem(1024, "my_buf", NULL);
  ck_assert_ptr_ne(mem, NULL);
  pid_t sharedpid = master_pid(mem);
  ck_assert_int_eq(pid, sharedpid);
  err = close_shared_mem(mem);
  ck_assert_int_eq(err, 0);
}
END_TEST

START_TEST(test_unused_exit)
{
  int err;
  shared_mem_t *mem = init_shared_mem(1024, "my_buf", NULL);
  ck_assert_ptr_ne(mem, NULL);
  wait_shared_client_exit(mem);
  // no lock!
  err = close_shared_mem(mem);
  ck_assert_int_eq(err, 0);
}
END_TEST

void *test_client_count_producer(void *arg) {
  (void)(arg);
  shared_mem_t *mem = init_shared_mem(1024, "my_buf", NULL);
  ck_assert_ptr_ne(mem, NULL);
  wait_shared_client_init(mem);
  // has the client!
  int clients = clients_shared_mem(mem);
  ck_assert_int_eq(clients, 1);

  wait_shared_client_exit(mem);
  clients = clients_shared_mem(mem);
  ck_assert_int_eq(clients, 0);

  close_shared_mem(mem);
  pthread_exit(NULL);
}

void *test_client_count_consumer(void *arg) {
  (void)(arg);
  usleep(100);
  shared_mem_t *mem = init_link_shared_mem(1024, "my_buf", NULL);
  ck_assert_ptr_ne(mem, NULL);

  usleep(100);
  close_link_shared_mem(mem);
  pthread_exit(NULL);
}

START_TEST(test_client_count)
{
  pthread_t wr, rd;
  pthread_create(&wr, NULL, test_client_count_producer, NULL);
  pthread_create(&rd, NULL, test_client_count_consumer, NULL);

  pthread_join(rd, NULL);
  pthread_join(wr, NULL);
}
END_TEST

Suite *sharedmem_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Shared Memore");
    tc_core = tcase_create("BufferShallock");

    tcase_add_test(tc_core, test_read_write);
    tcase_add_test(tc_core, test_master_pid);
    tcase_add_test(tc_core, test_unused_exit);
    tcase_add_test(tc_core, test_client_count);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;
    s = sharedmem_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
