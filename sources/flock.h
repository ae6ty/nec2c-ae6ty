#ifndef __FLOCK_H_
#define __FLOCK_H_

#include <pthread.h>
#include <errno.h>

typedef int flockattr_t;
typedef struct
{
  pthread_mutex_t mutex;
  pthread_cond_t corral;
  pthread_cond_t shepherd;
  int waiting;
  int size;
  pthread_t *threads;
  int cancel;
} flock_t;

int flock_init(flock_t *flock, const flockattr_t *attr, unsigned int count, void *sheep(void *));
int flock_destroy(flock_t *flock);

int flock_gather(flock_t *flock);
void flock_openGate(flock_t *flock);

int flock_destroy(flock_t *flock);

void flock_releaseFlock(flock_t *flock);
void flock_waitForFlock(flock_t *flock);
void flock_readyForWork(flock_t *flock);
#endif
