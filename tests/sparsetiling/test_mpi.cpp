/*
 *  test_mpi.cpp
 *
 * Check the correctness of MPI execution using a small, hand-coded mesh
 */

#include <mpi.h>

#include "inspector.h"
#include "executor.h"
#include "common.hpp"

int main (int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  const int nMPI = 2;
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (size > nMPI) {
    std::cout << "This test is only executable with at most 2 processes. Exiting." << std::endl;
    MPI_Finalize();
    return 0;
  }

  std::cout << "MPI Rank " << rank << ": loading mesh..." << std::endl;
  ExampleMeshMPI* mesh = example_mpi_mesh(RECT, rank);
  std::cout << "MPI Rank " << rank << ": loaded!" << std::endl;

  /*
   * Sample program structure:
   * - loop over cells (PL0):
   *     incr indirectly vertices (+=1)
   * - loop over vertices (PL1):
   *     pow to 2
   * - loop over cells (PL2):
   *     incr indirectly vertices (+=1)
   * - loop over vertices (PL3):
   *     pow to 2
   */

  // sets
  set_t* vertices = set("vertices", mesh->vertices, mesh->vertices_halo, 0);
  set_t* cells = set("cells", mesh->cells, mesh->cells_halo, 0);

  // maps
  map_t* c2vMap = map("c2v", cells, vertices, mesh->c2v, mesh->c2vSize);

  // descriptors
  desc_list pl0Desc ({desc(c2vMap, INC)});
  desc_list pl1Desc ({desc(DIRECT, READ),
                      desc(DIRECT, WRITE)});
  desc_list pl2Desc ({desc(c2vMap, INC)});
  desc_list pl3Desc ({desc(DIRECT, READ),
                      desc(DIRECT, WRITE)});

  // inspector
  const int tileSize = 3;
  inspector_t* insp = insp_init(tileSize, ONLY_MPI);

  insp_add_parloop (insp, "pl0", cells, &pl0Desc);
  insp_add_parloop (insp, "pl1", vertices, &pl1Desc);
  insp_add_parloop (insp, "pl2", cells, &pl2Desc);
  insp_add_parloop (insp, "pl3", vertices, &pl3Desc);

  const int seed = 0;
  insp_run (insp, seed);

  for (int i = 0; i < nMPI; i++) {
    if (i == rank) {
      insp_print (insp, HIGH);
      generate_vtk (insp, HIGH, vertices, mesh->coords, DIM2, rank);
    }
    MPI_Barrier(MPI_COMM_WORLD);
  }

  // executor
  executor_t* exec = exec_init (insp);

  // free memory
  insp_free (insp);
  exec_free (exec);

  delete mesh;

  MPI_Finalize();
  return 0;
}
