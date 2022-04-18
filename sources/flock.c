#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include "flock.h"
#include <stdlib.h>

int flock_init(flock_t *flock, const flockattr_t *attr, unsigned int sz, void *sheep(void *)) {
  if(sz == 0) {
    errno = EINVAL;
    return -1;
  }
  if(pthread_mutex_init(&flock->mutex, 0) < 0) {
    return -1;
  }
  if(pthread_cond_init(&flock->corral, 0) < 0) {
    pthread_mutex_destroy(&flock->mutex);
    return -1;
  }
  if(pthread_cond_init(&flock->shepherd, 0) < 0) {
    pthread_mutex_destroy(&flock->mutex);
    return -1;
  }
  flock->cancel = 0;
  flock->size = sz;
  flock->waiting = 0;
  flock->threads = malloc(sz*sizeof(*flock->threads));
  for (int i=0;i<flock->size;++i) {
    pthread_create(flock->threads+i,NULL, sheep, flock->threads+i);
  }
  return 0;
}

int flock_destroy(flock_t *flock) {
  for (int i=0;i<flock->size;++i) {
    pthread_join(flock->threads[i],NULL);
  }

  pthread_cond_destroy(&flock->corral);
  pthread_cond_destroy(&flock->shepherd);
  pthread_mutex_destroy(&flock->mutex);
  return 0;
}
int flock_gather(flock_t *flock) {
  if (flock->size == 1)
    return 1;

  pthread_mutex_lock(&flock->mutex);
  ++(flock->waiting);
  if(flock->waiting >= flock->size) {
    flock->waiting = 0;
    //      pthread_cond_broadcast(&(flock->corral)); // done 'shepherd'
    return 1;
  }
  pthread_cond_wait(&flock->corral, &(flock->mutex));
  pthread_mutex_unlock(&flock->mutex);
  return 0;
}

void flock_openGate(flock_t *flock) {
  if (flock->size != 1) {
    pthread_cond_broadcast(&(flock->corral));
    pthread_mutex_unlock(&flock->mutex);
  }
}

void flock_readyForWork(flock_t *flock) {
  pthread_mutex_lock(&flock->mutex);
  ++(flock->waiting);
  if(flock->waiting >= flock->size)
    pthread_cond_signal(&(flock->shepherd));
  pthread_cond_wait(&flock->corral, &(flock->mutex));
  pthread_mutex_unlock(&flock->mutex);
  if (flock->cancel)
    pthread_exit(NULL);
}

void flock_releaseFlock(flock_t *flock) {
  pthread_mutex_lock(&flock->mutex);
  flock->waiting = 0;
    pthread_cond_broadcast(&(flock->corral));
  pthread_mutex_unlock(&flock->mutex);
}

void flock_waitForFlock(flock_t *flock) {
  pthread_mutex_lock(&flock->mutex);
  if (flock->waiting != flock->size)
    pthread_cond_wait(&flock->shepherd, &(flock->mutex));
  pthread_mutex_unlock(&flock->mutex);
}

#ifdef DOTEST

int numSheep = 4;
static flock_t *flock;
static flockattr_t attr;

void *sheep(void *arg) {
  int me = (pthread_t *)arg - flock->threads;
  printf("me %d\n",me);
  fflush(stdout);
  while (1) {
    flock_readyForWork(flock);
    printf("working %d\n",me);
    fflush(stdout);
  }
}

int main(int ac, char **av) {
  flock = malloc(sizeof(*flock));
  int ret = flock_init(flock, &attr, numSheep,sheep);
  if (ret != 0)
    return ret;
  for (int i=0;i<5;++i) {
    flock_waitForFlock(flock);
    printf("shepherd\n");
    fflush(stdout);
    flock_releaseFlock(flock);
  }
  flock_waitForFlock(flock);
  printf("ending work\n");
  fflush(stdout);
  flock->cancel = 1;
  flock_releaseFlock(flock);
  flock_destroy(flock);
  fflush(stdout);
  fflush(stderr);
  return 0;
}
#endif

