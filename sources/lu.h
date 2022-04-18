#ifndef __LU_HEADER__
#define __LU_HEADER__
#ifndef Complex
#define Complex complex double
#endif
typedef int (*lu_spawn_t)(Complex *array,int stride, int n, int block, int *p, int r);
extern int lu_multiFactor(Complex *array, int stride, int n, int *p, int blockSize, int numProc);
extern int lu_factor(Complex *array, int stride, int n, int *p);
extern void lu_solve(Complex *array, int stride, int n, int *p, Complex *x);
extern void lu_printMatrix(char *msg, Complex *array, int stride, int n, int *p);
extern void lu_printRow(char *msg, Complex *array, int n, int tag);
extern void lu_copyArray(Complex *to, Complex *from, int n);
extern Complex *lu_multiply(Complex *array, int stride, int n, Complex *v);

extern int MPNumProcessors;
extern int MPBlockSize;
#endif
