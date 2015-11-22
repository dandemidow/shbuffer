#include "sharedmem/buffer.h"

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

static char *get_sem_name(char *name, char *semname) {
  char *sem_name = malloc(strlen(name) + strlen(semname)+2);
  sem_name[0] = '/';
  sem_name[1] = '\0';
  strcat(sem_name, name);
  strcat(sem_name, semname);
  return sem_name;
}

static void init_shared_core(shared_mem_t *shm, size_t buf_size, char *name) {
  shm->exit_sem_name = get_sem_name(name, "_sem_e");
  shm->buf_size = buf_size;
  shm->buf_name = malloc(strlen(name));
  strcpy(shm->buf_name, name);
}

static int init_mmap_core(shared_mem_t *shm) {
  shm->addr = mmap(0, shm->buf_size +1, PROT_WRITE|PROT_READ, MAP_SHARED, shm->shm_fd, 0);
  if ( shm->addr < (char*)0 ) {
    fprintf(stderr, "mmap error\n");
    return -1;
  }
  return 0;
}

int init_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name) {
  init_shared_core(shm, buf_size, name);

  if ( (shm->exit_sem = sem_open(shm->exit_sem_name, O_CREAT, 0777, 0)) == SEM_FAILED ) {
    fprintf(stderr, "exit sem open error\n");
    return -1;
  }
  int sem_val;
  do {
    if ( sem_getvalue(shm->exit_sem, &sem_val) < 0 ) {
      fprintf(stderr, "exit sem init error\n");
      return -1;
    }
    if ( sem_val ) sem_wait(shm->exit_sem);
  } while (sem_val != 0);

  shm->shm_fd = shm_open(shm->buf_name, O_CREAT|O_RDWR, 0777);
  if ( shm->shm_fd < 0 ) {
    fprintf(stderr, "shm open error\n");
    return -1;
  }
  if ( ftruncate(shm->shm_fd, shm->buf_size +1) == -1 ) {
    fprintf(stderr, "ftruncate error\n");
    return -1;
  }
  return init_mmap_core(shm);
}

int init_link_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name) {
  init_shared_core(shm, buf_size, name);

  if ( (shm->exit_sem = sem_open(shm->exit_sem_name, 0)) == SEM_FAILED ) {
    fprintf(stderr, "exit sem open error\n");
    return -1;
  }

  shm->shm_fd = shm_open(shm->buf_name, O_RDWR, 0777);
  if ( shm->shm_fd < 0 ) {
    fprintf(stderr, "shm open error\n");
    return -1;
  }
  return init_mmap_core(shm);
}

void *shared_buffer(shared_mem_t *shm) {
  return (void*)shm->addr;
}

static void close_shared_core(shared_mem_t *shm) {
  if ( munmap(shm->addr, shm->buf_size) < 0 ) {
    fprintf(stderr, "munmap error\n");
  }
  if ( close(shm->shm_fd) < 0 ) {
    fprintf(stderr, "close shared memory error\n");
  }
  free(shm->buf_name);
  free(shm->exit_sem_name);
}

int close_shared_buffer(shared_mem_t *shm) {
  close_shared_core(shm);
  shm_unlink(shm->buf_name);
  if ( sem_close(shm->exit_sem) < 0 ) {
    fprintf(stderr, "exit sem close error\n");
  }
  sem_unlink(shm->exit_sem_name);
  return 0;
}

int close_link_shared_buffer(shared_mem_t *shm) {
  close_shared_core(shm);
  sem_post(shm->exit_sem);
//  printf("sem post after\n");
  if ( sem_close(shm->exit_sem) < 0 ) {
    fprintf(stderr, "exit sem close error\n");
  }
  return 0;
}
