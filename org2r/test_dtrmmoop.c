// We get exact accuracy when comparing against reference LAPACK
// but when using AOCL we get errors as large as 1e-10. Not sure why
// Probably just a different dtrmm algorithm. May investigate? 
#ifdef USE_AOCL
    #define SOURCE "AOCL"
#else
    #define SOURCE "REF_LAPACK"
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
int main(int argc, char *argv[]) {
    // integer variables
    int info, lda, ldt, m, n, k, lwork, nb, i, j;
    int workQuery = -1;
    // double variables
    double *A, *As, *T, *work, *workS=NULL;
    double normA;
    double elapsed_refL, norm_orth_1, norm_repres_1;
    double zero = 0;
    double one = 1;
    double negOne = -1;
    // struct for help with timing
    struct timeval tp;
    // character variables
    char aChar = 'A';
    char fChar = 'F';
    char lChar = 'L';
    char nChar = 'N';
    char rChar = 'R';
    char tChar = 'T';
    char uChar = 'U';

    // Dummy value that is used when calling fortran
    // functions that take characters from C
    size_t dummy = 0;

    m = 30;
    n = 20;
    lda = -1;
    ldt = -1;

    for(i = 1; i < argc; ++i){
        if( strcmp( *(argv + i), "-ldt") == 0) {
            ldt  = atoi( *(argv + i + 1) );
            i++;
        }
        if( strcmp( *(argv + i), "-lda") == 0) {
            lda  = atoi( *(argv + i + 1) );
            i++;
        }
        if( strcmp( *(argv + i), "-m") == 0) {
            m  = atoi( *(argv + i + 1) );
            i++;
        }
        if( strcmp( *(argv + i), "-n") == 0) {
            n  = atoi( *(argv + i + 1) );
            i++;
        }
    }

    if( lda < 0 ) lda = m;
    if( ldt < 0 ) ldt = m;

    char *source = SOURCE;
    printf("dtrmm vs dtrmmoop %s: m = %4d, n = %4d, lda = %4d, ldt = %4d\n",source, m, n, lda, ldt);

    // Generate 2 random matrices. A and T
    // Even though the subroutine we are testing will 
    //  be multiplying a triangular matrix we want to 
    //  fill up the entire matrix so that we can test 
    //  it properly takes advantage of this fact
    // A will be m by n
    A  = (double *) malloc (lda * n * sizeof(double));
    As = (double *) malloc (lda * n * sizeof(double));
    // T will be m by m
    T  = (double *) malloc (ldt * m * sizeof(double));
    // Create a workspace that will store the output of dtrmmoop
    work = (double *) malloc (m * n * sizeof(double));


    for (i = 0; i < lda * n; i++)
        A[i] = (double) rand() / (double) (RAND_MAX) - 0.5e+00;
    for (i = 0; i < ldt * m; i++)
        T[i] = (double) rand() / (double) (RAND_MAX) - 0.5e+00;
    // Copy over A into As. Even though ours is out of 
    // place, this will allow us to check for no change
    // in A
    dlacpy_(&aChar, &m, &n, A, &lda, As, &lda, dummy);
    // Call dtrmmoop
    // In order to compare against dtrmm, we need to pass in 0 as alpha
    double alpha = 0;
    dtrmmoop_(&m, &n, A, &lda, T, &ldt, &alpha, work, &m);
    // Make sure that A = As
    // So we can just check if there is any elements are different
    for (int i = 0; i < lda * n; i++)
        if (A[i] != As[i])
            // report to user
            printf("Difference in A at index %4d\n", i);
    
    // Assuming that A = As, we now call dtrmm with A and T
    dtrmm_(&lChar, &uChar, &nChar, &uChar, &m, &n, &one, T, &ldt, A, &lda, dummy, dummy, dummy, dummy);

    // Compare A and work to make sure they are the same. Since we are only needing to compute the Frobenius norm
    double norm_diff_F = 0.0;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            double tmp = work[i + j*m] - A[i + j*lda];
            norm_diff_F += tmp*tmp;
        }
    }
    // Actually computing the norm as the optimized version may be a different algorithm leading to
    // different roundoff errors.
    norm_diff_F = sqrt(norm_diff_F);
    printf("error for alpha =%4.1lf: %1.7e\n", alpha, norm_diff_F);

    // Now, we want to check the new behavior. IE does dtrmmoop correctly use alpha != 0
    
    // Set work to be something random.
    for (i = 0; i < m * n; i++)
        work[i] = (double) rand() / (double) (RAND_MAX) - 0.5e+00;

    // Make a new matrix that will hold a copy of work
    workS = (double *) malloc(m * n * sizeof(double));
    // Copy work into workS
    dlacpy_(&aChar, &m, &n, work, &m, workS, &m, dummy);
    // Copy As back into A
    dlacpy_(&aChar, &m, &n, As, &lda, A, &lda, dummy);

    // Compute ours
    alpha = 1;
    dtrmmoop_(&m, &n, A, &lda, T, &ldt, &alpha, work, &m);

    // Compute dtrmm
    dtrmm_(&lChar, &uChar, &nChar, &nChar, &m, &n, &one, T, &ldt, A, &lda, dummy, dummy, dummy, dummy);
    for (i = 0; i < m; i++)
        for (j = 0; j < n; j++)
            workS[i + j*m] += A[i + j*lda];

    // Compare the matrices, work and workS. These should be the exact same for REFLAPACK. 
    // May differ for OPTBLAS
    norm_diff_F = 0;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            double tmp = work[i+j*m] - workS[i + j*m];
            norm_diff_F += tmp * tmp;
        }
    }
    norm_diff_F = sqrt(norm_diff_F);
    printf("error for alpha =%4.1lf: %1.7e\n", alpha, norm_diff_F);

    // Do above but with alpha != 0 and alpha != 1
    for (i = 0; i < m * n; i++)
        work[i] = (double) rand() / (double) (RAND_MAX) - 0.5e+00;

    dlacpy_(&aChar, &m, &n, work, &m, workS, &m, dummy);
    // Copy As back into A
    dlacpy_(&aChar, &m, &n, As, &lda, A, &lda, dummy);

    // Compute ours
    alpha = 3.5;
    dtrmmoop_(&m, &n, A, &lda, T, &ldt, &alpha, work, &m);

    // Compute dtrmm
    dtrmm_(&lChar, &uChar, &nChar, &nChar, &m, &n, &one, T, &ldt, A, &lda, dummy, dummy, dummy, dummy);
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            workS[i + j*m] = alpha * workS[i+j*m] + A[i+j*lda];
        }
    }

    // Compare the matrices, work and workS. These should be the exact same for REFLAPACK. 
    // May differ for OPTBLAS
    norm_diff_F = 0;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            double tmp = work[i+j*m] - workS[i + j*m];
            norm_diff_F += tmp * tmp;
        }
    }
    norm_diff_F = sqrt(norm_diff_F);
    printf("error for alpha =%4.1lf: %1.7e\n", alpha, norm_diff_F);

    // Note: As long as all 3 errors are roughly similar, then we should be alright.

    // free memory
    free(A);
    free(As);
    free(T);
    free(work);
    free(workS);
}
