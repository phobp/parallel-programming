/**
 * mc_pi.c
 *
 * CS 470 OpenMP Lab
 * Based on IPP Programming Assignment 5.2
 *
 * Names: Brendan Pho, Wesley Llamas
 * 
 * ===== ANALYSIS =====
 *
 *  In the parallel version of the Monte Carlo Program we added a pragma
 * at the very beginning of the throw_darts() function to fork the program
 * into a number of threads in order to handle the loop and updates later
 * in the function. We added a reduction for the darts_in_circle variable
 * because at the end of the function, we need to add all the darts that are
 * in the circle from all threads. We also state that the total_darts variable
 * is a variable that is shared among all threads because all the threads need
 * to know how many darts there are in order to execute the loop. The
 * darts_in_circle variable is also implied to be shared. Finally, we add a 
 * pragma for the for loop so that the for loop's iterations are split
 * equally among the processes.
 *
 *  The parallel version of the program performed much faster than the serial
 * version. For the serial version, it took 3.458 seconds to run the program.
 * For the parallel version, it took 2.438, 1.983, 1.403, 0.754, and 0.406 seconds
 * to run the program for 1, 2, 4, 8, and 16 threads respectively. The parallel
 * version's run times are cut in half as the amount of threads are doubled. This
 * shows that the program is strongly scalable. The estimates of pi are also
 * accurate, with the results being around 3.1415. Compared to using pthreads,
 * using OpenMP was much easier in terms of how much code was used. However,
 * thinking about what needed to be parallelized took around the same amount
 * of thought as using pthreads. Compared to the pthreads implementation,
 * the two programs performed about the same except when there was a thread
 * count of 1. With a thread count of 1, the OpenMP implementation was faster
 * with a run time of 2.438 seconds compared to the pthread's 3.443 seconds.
 *
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

#ifdef _OPENMP
#include <omp.h>
#endif

long total_darts = 0;           // dart count
long darts_in_circle = 0;       // number of hits

void throw_darts()
{
#   pragma omp parallel default(none) reduction(+:darts_in_circle) shared(total_darts)    
    {
        // seed pseudo-random number generator
        unsigned long seed = 0;
#ifdef _OPENMP
        seed = (unsigned long) omp_get_thread_num();
#endif
#       pragma omp for
        for (long dart = 0; dart < total_darts; dart++) {

            // throw a dart by generating a random (x,y) coordinate pair
            // using a basic linear congruential generator (LCG) algorithm
            // (see https://en.wikipedia.org/wiki/Linear_congruential_generator)
            seed = (1103515245*seed + 12345) % (1<<31);
            double x = (double)seed / (double)ULONG_MAX;
            seed = (1103515245*seed + 12345) % (1<<31);
            double y = (double)seed / (double)ULONG_MAX;
            double dist_sq = x*x + y*y;

            // update hit tracker
            if (dist_sq <= 1.0) {
                darts_in_circle++;
            }
        }
    }
}

int main(int argc, char* argv[])
{
    // check and parse command-line arguments
    if (argc != 2) {
        printf("Usage: %s <num-darts>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    total_darts = strtoll(argv[1], NULL, 10);

    START_TIMER(darts)

    // simulate dart throws
    throw_darts();

    STOP_TIMER(darts)

    // calculate pi
    double pi_est = 4 * darts_in_circle / ((double)total_darts);
    printf("Estimated pi: %e   Time elapsed: %.3lfs\n",
            pi_est, GET_TIMER(darts));

    // clean up
    return EXIT_SUCCESS;
}


