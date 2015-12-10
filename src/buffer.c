#include "sharedmem/buffer.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>

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
  shm->init_sem_name = get_sem_name(name, "_sem_i");
  shm->buf_size = buf_size;
  shm->buf_name = malloc(strlen(name));
  strcpy(shm->buf_name, name);
}

static int init_mmap_core(shared_mem_t *shm) {
  shm->addr = mmap(0, shm->buf_size +1, PROT_WRITE|PROT_READ, MAP_SHARED, shm->shm_fd, 0);
  if ( shm->addr < (char*)0 ) {
    fprintf(stderr, "mmap error\n");
    return BUF_MMAP_ERR;
  }
  return BUF_SUCCESS;
}

static sem_t *init_shared_sem(char *sem_name) {
  sem_t *sem;
  if ( (sem = sem_open(sem_name, O_CREAT, 0777, 0)) == SEM_FAILED ) {
    return NULL;
  }
  int sem_val;
  do {
    if ( sem_getvalue(sem, &sem_val) < 0 ) {
      return NULL;
    }
    if ( sem_val ) sem_wait(sem);
  } while (sem_val != 0);
  return sem;
}

int init_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name) {
  init_shared_core(shm, buf_size, name);

  if ( (shm->init_sem = init_shared_sem(shm->init_sem_name)) == NULL ) {
    return BUF_INIT_ISEM_ERR;
  }

  if ( (shm->exit_sem = init_shared_sem(shm->exit_sem_name)) == NULL ) {
    return BUF_INIT_ESEM_ERR;
  }

  shm->shm_fd = shm_open(shm->buf_name, O_CREAT|O_RDWR, 0777);
  if ( shm->shm_fd < 0 ) {
    return BUF_SHM_OPEN_ERR;
  }
  if ( ftruncate(shm->shm_fd, shm->buf_size +1) == -1 ) {
    return BUF_SHM_TRUNC_ERR;
  }
  return init_mmap_core(shm);
}

int init_link_shared_buffer(shared_mem_t *shm, size_t buf_size, char *name) {
  init_shared_core(shm, buf_size, name);

  if ( (shm->exit_sem = sem_open(shm->exit_sem_name, 0)) == SEM_FAILED ) {
    return BUF_INIT_ESEM_ERR;
  }

  if ( (shm->init_sem = sem_open(shm->init_sem_name, 0)) == SEM_FAILED ) {
    return BUF_INIT_ISEM_ERR;
  }

  shm->shm_fd = shm_open(shm->buf_name, O_RDWR, 0777);
  if ( shm->shm_fd < 0 ) {
    return BUF_SHM_OPEN_ERR;
  }
  return init_mmap_core(shm);
}

void *shared_buffer(shared_mem_t *shm) {
  return (void*)shm->addr;
}

static int close_shared_core(shared_mem_t *shm) {
  if ( munmap(shm->addr, shm->buf_size) < 0 ) {
    return BUF_UNMMAP_ERR;
  }
  if ( close(shm->shm_fd) < 0 ) {
    return BUF_SHM_CLOSE_ERR;
  }
  free(shm->buf_name);
  return BUF_SUCCESS;
}

int close_shared_buffer(shared_mem_t *shm) {
  int err = close_shared_core(shm);
  if ( err < 0 ) return err;
  shm_unlink(shm->buf_name);
  if ( sem_close(shm->init_sem) < 0 ) {
    return BUF_EXIT_ISEM_ERR;
  }
  sem_unlink(shm->init_sem_name);
  free(shm->init_sem_name);
  if ( sem_close(shm->exit_sem) < 0 ) {
    return BUF_EXIT_ESEM_ERR;
  }
  sem_unlink(shm->exit_sem_name);
  free(shm->exit_sem_name);
  return BUF_SUCCESS;
}

int close_link_shared_buffer(shared_mem_t *shm) {
  int err = close_shared_core(shm);
  if ( err < 0 ) return err;
  sem_post(shm->exit_sem);
  free(shm->exit_sem_name);
  free(shm->init_sem_name);

  if ( sem_close(shm->init_sem) < 0 ) {
    return BUF_EXIT_ISEM_ERR;
  }
  if ( sem_close(shm->exit_sem) < 0 ) {
    return BUF_EXIT_ESEM_ERR;
  }
  return BUF_SUCCESS;
}
