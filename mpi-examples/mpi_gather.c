/*
 * CS 470 MPI Collective Gather Example
 */

#include <stdio.h>
#include <mpi.h>

#define MAX_SIZE 256

int main(int argc, char *argv[])
{
    int my_rank, num_ranks;
    int data[MAX_SIZE];

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

    // initialize 'data' to dummy values
    for (int i = 0; i < num_ranks; i++) {
        data[i] = -1;
    }

    // send rank id from every process to process 0
    MPI_Gather(&my_rank, 1, MPI_INT,
                   data, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // print 'data' at process 0
    if (my_rank == 0) {
        printf("Received data in rank %d: ", my_rank);
        for (int i = 0; i < num_ranks; i++) {
            printf("%d ", data[i]);
        }
        printf("\n");
    }

    MPI_Finalize();
    return 0;
}
