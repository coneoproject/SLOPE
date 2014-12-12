/*
 *  test_inspector.cpp
 *
 * Check the correctness of the inspector using a simple, small mesh
 */

#include "inspector.h"
#include "common.hpp"

using namespace std;

int main ()
{
  const int nLoops = 3;
  ExampleMesh mesh = example_mesh(RECT);
  /*
   * Sample program structure:
   * - loop over edges (PL0):
   *     read indirectly vertices
   *     write directly edges
   * - loop over cells (PL1):
   *     read indirectly vertices
   *     read directly edges
   *     increment indirectly vertices
   * - loop over edges (PL2):
   *     read indirectly vertices
   *     write directly edges
   */

  // Sets
  set_t* vertices = set("vertices", mesh.vertices);
  set_t* edges = set("edges", mesh.edges);
  set_t* cells = set("cells", mesh.cells);

  // Maps
  map_t* e2vMap = map(edges, vertices, mesh.e2v, mesh.e2vSize);
  map_t* c2vMap = map(cells, vertices, mesh.c2v, mesh.c2vSize);

  // Descriptors
  desc_list pl0Desc ({desc(e2vMap, READ),
                      desc(DIRECT, WRITE)});
  desc_list pl1Desc ({desc(c2vMap, READ),
                      desc(DIRECT, READ),
                      desc(c2vMap, INC)});
  desc_list pl2Desc ({desc(e2vMap, READ),
                      desc(DIRECT, WRITE)});

  const int tileSize = 4;
  inspector_t* insp = insp_init(tileSize, SEQUENTIAL);

  insp_add_parloop (insp, "pl0", edges, &pl0Desc);
  insp_add_parloop (insp, "pl1", cells, &pl1Desc);
  insp_add_parloop (insp, "pl2", edges, &pl2Desc);

  const int seed = 1;
  insp_run (insp, seed);

  insp_print (insp, MEDIUM);
  return 0;
}
