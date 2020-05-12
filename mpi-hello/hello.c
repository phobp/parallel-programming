/*
 * CS 470 Example
 *
 * Basic MPI hello-world demo.
 */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    char mpi_name[MPI_MAX_PROCESSOR_NAME];
    int  mpi_name_len;
    int  mpi_rank;
    int  mpi_size;

    MPI_Init(&argc, &argv);

    MPI_Get_processor_name(mpi_name, &mpi_name_len);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    printf("Hello from process %2d / %d (%s)!\n", mpi_rank+1, mpi_size, mpi_name);

    MPI_Finalize();

    return 0;
}

