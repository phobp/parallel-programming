/*
 * CS 470 Matrix Multiplication Example
 *
 * Demonstrates dense matrix access patterns and OpenMP parallelization.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "omp_timer.h"          // custom timing code

typedef int      data_t;        // matrix data type
typedef unsigned idx_t;         // index data type

/*
 * Prints a square matrix A to standard output in a fixed-width format.
 */
void print_matrix(data_t *A, idx_t n)
{
    idx_t i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            printf("%8d ", A[i*n+j]);
        }
        printf("\n");
    }
}

/*
 * Compare two n x n matrices to see if they are identical.
 */
bool equal_matrices(data_t *A, data_t *B, idx_t n)
{
    idx_t i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            if (A[i*n+j] != B[i*n+j]) {
                return false;
            }
        }
    }
    return true;
}

/*
 * Computes R = A*B (assumes all matrices are n x n)
 * (serial version)
 */
void multiply_matrices(data_t *A, data_t *B, data_t *R, idx_t n)
{
    idx_t i, j, k;
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            R[i*n+j] = 0;
            for (k = 0; k < n; k++) {
                R[i*n+j] += A[i*n+k] * B[k*n+j];
            }
        }
    }
}

/*
 * Computes R = A*B (assumes all matrices are n x n)
 * (parallel version)
 */
void par_multiply_matrices(data_t *A, data_t *B, data_t *R, idx_t n)
{
    // TODO: finish this method by adding OpenMP pragmas
    idx_t i, j, k;
#ifdef _OPENMP
#   pragma omp paralell for default(none) shared(A,B,R,n,) private(i,j,k)
#endif
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            R[i*n+j] = 0;
            for (k = 0; k < n; k++) {
                R[i*n+j] += A[i*n+k] * B[k*n+j];
            }
        }
    }
}

int main(int argc, char *argv[])
{
    // parse command-line parameters
    if (argc != 2) {
        printf("Usage: %s <n>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    idx_t n = strtoul(argv[1], NULL, 10);

    // allocate matrix memory
    data_t *a = (data_t*)malloc(n*n * sizeof(data_t));
    data_t *b = (data_t*)malloc(n*n * sizeof(data_t));
    data_t *c = (data_t*)calloc(n*n,  sizeof(data_t));
    data_t *r = (data_t*)calloc(n*n,  sizeof(data_t));
    if (!a || !b || !c || !r) {
        printf("Couldn't allocate memory!\n");
        exit(EXIT_FAILURE);
    }

    // initialize input matrices to random numbers
    srand(42);
    for (idx_t i=0; i<n*n; i++) {
        a[i] = rand() % 100;
        b[i] = rand() % 100;
    }

    // do parallel multiplication
    START_TIMER(mult)
    par_multiply_matrices(a, b, c, n);
    STOP_TIMER(mult)

    // calculate reference solution
    multiply_matrices(a, b, r, n);

#   ifdef DEBUG
    // print matrices
    printf("A =\n");
    print_matrix(a, n);
    printf("B =\n");
    print_matrix(b, n);
    printf("A*B =\n");
    print_matrix(c, n);
    printf("soln =\n");
    print_matrix(r, n);
#   endif

    // find thread count
    int nthreads = 1;
#   ifdef _OPENMP
#   pragma omp parallel
#   pragma omp single
    nthreads = omp_get_num_threads();
#   endif

    // print results
    printf("Nthreads=%2d  TEST=%s  MULT=%8.4f\n", nthreads,
            equal_matrices(c, r, n) ? "pass" : "FAIL", GET_TIMER(mult));

    // clean up
    free(a);
    free(b);
    free(c);
    free(r);
    return EXIT_SUCCESS;
}

