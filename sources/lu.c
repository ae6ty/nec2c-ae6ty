#include <stdio.h>
#include <stdlib.h>
#include <complex.h>

#include "lu.h"
#include "flock.h"

static int debug = 0;
void lu_printRow(char *msg, Complex vec[], int n, int p) {
  printf("%s",msg);
  for (int c=0;c<n;++c)
    printf("(% f,%f) ",creal(vec[c]),cimag(vec[c]));
  if (p != -1)
    printf("%d",p);
  printf("\n");
}

#define PIVOT_TOO_SMALL 1e-10

void lu_printMatrix(char *msg, Complex *array, int stride, int n, int *p) {
  printf("%s:\n",msg);
  for (int r=0;r<n;++r)
    lu_printRow("    ",array+stride*r,n,p[r]);
}

typedef struct {
  Complex *array;
  int stride;
  int n;
  int block;
  int *p;
  int start;
  _Atomic int sliceNum;
} pasture_t;
pasture_t pasture;


static int pickPivot(Complex *array, int stride, int n, int *p, int start) {
  double maxMag = 1e-100;
  int maxRow = -1;
  for (int r=start;r<n;++r) {
    Complex c = array[p[r]*stride+start];
    double tmp = creal(c)*creal(c)+cimag(c)*cimag(c);
    if (tmp > maxMag) {
      maxMag = tmp;
      maxRow = r;
    }
  }
  if (maxRow != -1) {
    int tmp = p[start];
    p[start] = p[maxRow];
    p[maxRow] = tmp;
  }
  return (maxMag < PIVOT_TOO_SMALL);
}

int lu_factor(Complex *array, int stride, int n, int *p) {
  int countErrors = 0;
  for (int r=0;r<n;++r) {
    Complex *ref = array + p[r]*stride;
    for (int r1=r+1;r1<n;++r1) {
      Complex *row = array + p[r1]*stride;
      Complex factor = row[r]/ref[r];
      row[r] = factor;
      for (int k=r+1;k<n;++k)
	row[k] -= ref[k]*factor;
    }
    if (debug) lu_printMatrix("good after row",array,stride,n,p);
    if (r+1 < n)
      countErrors += pickPivot(array,stride,n,p,r+1);
  }
  return countErrors;
}

void slice(Complex *array,int stride, int n, int block, int *p, int start, int slice) {
  int sliceStart = start + slice*block;
  if (sliceStart >= n)
    return;
  int sliceEnd =  sliceStart + block;
  //  printf("slice row:%d col:%d to:%d\n",start,sliceStart,sliceEnd);
  if (sliceEnd > n)
    sliceEnd = n;

  // get array of U rows
  Complex **refs = (Complex **)alloca(block*sizeof(Complex *));
  for (int b=0;b<block;++b) {
    int r = start-block+b;
    refs[b] = array+p[r]*stride;
  }

  //do the triangluar part
  for (int b=0;b<block;++b) {
    int r = start-block+b;
    //    Complex *ref = array+p[r]*stride;
    Complex *ref = refs[b];
    for (int r1=r+1;r1<start;++r1) {
      Complex *row = array + p[r1]*stride;
      Complex factor = row[r];
      //      for (int k=start+block;k<n;++k) {
      for (int k=sliceStart;k<sliceEnd;++k) {
	row[k] -= ref[k]*factor;
      }
    }
  }

  /*
   * transposes addin matrix
   */
  Complex *addins = (Complex *)alloca(block*block*sizeof(Complex));
  Complex *cptr = addins;
  for (int c=sliceStart;c<sliceEnd;++c) {
    for (int r=0;r<block;++r) {
      Complex tmp = refs[r][c];
      *cptr++ = tmp;
    }
  }
  // now do the remaining rows.
  for (int r = start;r < n;++r) {
    Complex *row = array + p[r]*stride;
    Complex *factors = row+start-block;
    Complex *addinCol = addins;
    for (int k=sliceStart;k<sliceEnd;++k) { // for each column
      Complex rslt = row[k];
      for (int b=0;b<block;++b) {
	rslt -= addinCol[b]*factors[b];
      }
      row[k] = rslt;
      addinCol += block;
    }
  }
}


static flock_t *flock;
static flockattr_t attr;

void *sheep(void *arg) {
  //  int me = (pthread_t *)arg - flock->threads;
  while (1) {
    flock_readyForWork(flock);
    int slc;
    while (1) {
      // the shepherd does the '0'th slice.
      slc = ++pasture.sliceNum;
      if (slc*pasture.block > pasture.n)
	break;
      //      printf("sheep %d working on slice %d\n",me,slc);
      slice(pasture.array,pasture.stride,pasture.n,pasture.block,pasture.p,pasture.start,slc);
    }
  }
}

void freeFlock() {
  flock_waitForFlock(flock);
  flock->cancel = 1;
  flock_releaseFlock(flock);
  flock_destroy(flock);
  flock = NULL;
}
int initflock(int numSheep) {
  if (flock != NULL)
    return 0;

  flock = malloc(sizeof(*flock));
  int ret = flock_init(flock, &attr, numSheep,sheep);
  if (ret != 0)
    return ret;

  return ret;
}


int spawn(Complex *array,int stride, int n, int block, int *p, int start) {
  flock_waitForFlock(flock);

  pasture.array = array;
  pasture.stride = stride;
  pasture.n = n;
  pasture.block = block;
  pasture.p = (int *)malloc(n*sizeof(int));
  pasture.start = start;
  pasture.sliceNum = 0;
  for (int i=0;i<n;++i)
    pasture.p[i] = p[i];

  flock_releaseFlock(flock);

  int count = 0;

  //  printf("n %d, start %d, b %d\n",n,start,block);
  int slc = 0;
  //  for (int slc = 0; slc < numSlices;++slc) {
    slice(pasture.array,pasture.stride,pasture.n,pasture.block,pasture.p,pasture.start,slc);
  //  }

  return count;
}

#define magSquared(c) (creal(c)*creal(c)+cimag(c)*cimag(c))

int lu_factor1(Complex *array, int stride, int n, int block, int *p, lu_spawn_t spawn, int numSheep) {

  int numErrors = 0;
  numErrors = initflock(numSheep);
  if (numErrors)
    return numErrors;

  int endUAt = block;
  for (int r=0;r<n;++r) {
    Complex *ref = array + p[r]*stride;
    double max = 1e-100;
    int maxRow = -1;
    if (endUAt > n)
      endUAt = n;
    for (int r1=r+1;r1<n;++r1) {
      Complex *row = array + p[r1]*stride;
      Complex factor = row[r]/ref[r];
      row[r] = factor;
      for (int k=r+1;k<endUAt;++k)
	row[k] -= ref[k]*factor;
      Complex c = row[r+1];
      double tmp = magSquared(c);
      if (tmp > max) {
	maxRow = r1;
	max = tmp;
      }
    }
    if (r+1 == endUAt) {
      if (debug) lu_printMatrix("before spawn",array,stride,n,p);
      numErrors += spawn(array,stride,n,block,p,r+1);
      endUAt += block;
    }
    if (debug) lu_printMatrix("new  after row",array,stride,n,p);
    if (r+1 < n) {
      numErrors += (max < PIVOT_TOO_SMALL);
      if (maxRow != -1) {
	int tmp = p[r+1];
	p[r+1] = p[maxRow];
	p[maxRow] = tmp;
      }
    }
  }
  freeFlock();
  return numErrors;
}
int lu_multiFactor(Complex *array, int stride, int n, int *p, int blockSize, int numProc) {
  //  lu_factor1(array,N,N,8,p,spawn,4);
  //  fprintf(stderr,"multifactor %d %d\n",numProc,blockSize);
  return lu_factor1(array,stride,n,blockSize,p,spawn,numProc);
}

void lu_solve(Complex *array, int stride, int n, int *p, Complex *x) {
  Complex *y = malloc(n*sizeof(*array));
  for (int i=0;i<n;++i) {
    Complex *row = array+stride*p[i];
    Complex tmp = x[p[i]];
    for (int k=0;k<i;++k)
      tmp -= y[k]*row[k];
    y[i] = tmp;
  }
  for (int i=n-1;i>=0;--i) {
    Complex *row = array + p[i]*stride;
    Complex tmp = y[i];
    for (int k=n-1;k>i;--k)
      tmp -= x[k]*row[k];
    x[i] = tmp/row[i];
  }
  free(y);
}
Complex * lu_multiply(Complex *array, int stride, int n, Complex *v) {
  Complex *rval = malloc(n*sizeof(*array));
  for (int i=0;i<n;++i) {
    Complex dot = 0.0;
    for (int k=0;k<n;++k)
      dot += array[i*stride+k] * v[k];
    rval[i] = dot;
  }
  return rval;
}
#ifdef TESTLU
void lu_copyArray(Complex *to, Complex *from, int n) {
  for (int i=0;i<n;++i)
    to[i] = from[i];
}

//Complex initialArray[] = {1,1,-1, 1,-2,3, 2,3,1};
//Complex initialX[] = {4,-6,7};

Complex biginitialX[] = {
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    70,29,28,27,26, 25,24,23,22,21,
    60,29,28,27,26, 25,24,23,22,21,
    50,29,28,27,26, 25,24,23,22,21,
    40,29,28,27,26, 25,24,23,22,21,
    30,29,28,27,26, 25,24,23,22,21,
    20,19,18,17,16, 15,14,13,12,11,
    10, 9, 8, 7, 6,
    //  */
    5, 4, 3, 2, 1
};
Complex smallinitialX[] = {7,6,5,4,3,2,1};

//Complex initialArray[] = {
//  1,2,3,4,5,6,
//  7,8,9,10,11,12,
//  18,17,16,15,14,13,
//  19,20,21,22,23,24,
//  25,26,27,28,29,30,
//  31,32,33,34,35,36
//};
Complex *initialArray;
//Complex initialArray[N*N];
//Complex initialX[N];

static void randomize(Complex *a, int n) {
  for (int i=0;i<n;++i)
    *a++ = (Complex)(random()%100);
}
  static void initP(int *p,int n) {
  for (int i=0;i<n;++i)
    p[i] = i;
}
#define uint64_t u_int64_t
extern uint64_t getTimeStamp();
static double test(int N, int blockSize, int numProc) {
  long normalTime;
  long multiTime;
  initialArray = (Complex *)malloc(N*N*sizeof(Complex));
  randomize(initialArray,N*N);

  Complex *initialX = (Complex *)malloc(N*sizeof(Complex));
  randomize(initialX,N);
  Complex *array = malloc(N*N*sizeof(Complex));

  Complex x[N];
  int p[N];
  lu_copyArray(array,initialArray,N*N);
  uint64_t start = getTimeStamp();
  initP(p,N);
  lu_factor(array,N,N,p);
  uint64_t end = getTimeStamp();
  normalTime = (long)(end-start);

  lu_copyArray(array,initialArray,N*N);
  initP(p,N);
  start = getTimeStamp();
  int ret = lu_multiFactor(array,N,N,p,blockSize,numProc);
  if (ret != 0)
    fprintf(stderr,"Failure to factor\n");
  end = getTimeStamp();
  multiTime = (long)(end-start);


  double ratio = (double)normalTime/(double)multiTime;

  lu_copyArray(x,initialX,N);
  lu_solve(array,N,N,p,x);
  
  lu_copyArray(array,initialArray,N*N);
  Complex *verified = lu_multiply(array,N,N,x);

  int errors = 0;
  for (int i=0;i<N;++i) {
    Complex diff = verified[i]-initialX[i];
    if (magSquared(diff) > .0001) {
      ++errors;
      printf("diff at index %d %f %f\n",i,creal(diff),cimag(diff));
    }
  }
  free(initialArray);
  free(array);
  free(initialX);

  if (errors) {
    lu_printRow("x:",x,N,-1);
    lu_printRow("verified:",verified,N,-1);
    lu_printRow("expected:",initialX,N,-1);
    return -1;
  } else {
    //    printf("ALL OK!!!!!!\n");
  }
  return ratio;
}
int foomain(int ac, char **av) {
  test(16,32,3);
  fflush(stdout);
  fflush(stderr);
  return 1;
}
int main(int ac, char **av) {
  int bestBlockSize,bestNumProcs;
  double bestRatio = 0;
  fflush(stderr);
  fflush(stdout);
  printf("\tsize\tblocksize\tnumProcs\tratio\n");
  fflush(stdout);
  for (int N=16;N<=4096;N *= 2) {
    bestBlockSize = 0;
    bestNumProcs = 0;
    bestRatio = 0;
    for (int blockSize=8;blockSize<=64;blockSize *= 2) {
      for (int numProcs=2;numProcs<=32;) {
	double ratio = test(N,blockSize,numProcs);
	printf("\t%d\t%d\t%d\t%g\n",N,blockSize,numProcs,ratio);
	fflush(stdout);
	if (ratio > bestRatio) {
	  bestNumProcs = numProcs;
	  bestBlockSize = blockSize;
	  bestRatio = ratio;
	}
	if (numProcs < 8)
	  ++numProcs;
	else
	  numProcs *= 2;
      }
    }
    printf("BEST:\t%d\t%d\t%d\t%g\n",N,bestBlockSize,bestNumProcs,bestRatio);
  }
  return 0;
}
#endif
