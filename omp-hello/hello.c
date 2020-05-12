/*
 * CS 470 OpenMP Hello World Example
 *
 * Demonstrates basic OpenMP functionality.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[])
{
#   pragma omp parallel
    printf("Hello!\n");

    printf("Goodbye!\n");
    return EXIT_SUCCESS;
}

