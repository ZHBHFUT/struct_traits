#include <cstdio> // printf
#include "MPITypes.hpp"

//
// A demo for using struct_traits to create user defined MPI datatype automatically.
//

// a user defined data
struct A{
    double a0{};
    int    a1[2]{};
    char   a2{};
};

int main(int argc, char** argv)
{
    int rank = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    A a;
    if (rank == 0) {
        a.a0 = 1;
        a.a1[0] = 2; a.a1[1] = 3;
        a.a2 = 'A';
    }
    MPI_Bcast(&a, 1, mpi::Data<A>::type(), 0, MPI_COMM_WORLD);

    printf("[%d] a0=%lf, a1={%d,%d}, a2=%c\n", rank, a.a0, a.a1[0], a.a1[1], a.a2);

    MPI_Finalize();

    return 0;
}
