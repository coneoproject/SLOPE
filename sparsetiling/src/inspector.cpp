/*
 *  inspector.cpp
 *
 * Implement inspector routines
 */

#include <stdlib.h>

#include "inspector.h"
#include "utils.h"
#include "partitioner.h"

using namespace std;

inspector_t* insp_init (int tileSize)
{
  inspector_t* insp = (inspector_t*) malloc (sizeof(inspector_t));

  insp->tileSize = tileSize;
  insp->loops = (loop_list*) malloc (sizeof(loop_list*));

  insp->seed = -1;
  insp->itset2tile = NULL;

  return insp;
}

insp_info insp_add_parloop (inspector_t* insp, char* loopName, int setSize,
                            desc_list* descriptors)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  loop_t* loop = (loop_t*) malloc (sizeof(loop_t));
  loop->loopName = loopName;
  loop->setSize = setSize;
  loop->descriptors = descriptors;

  insp->loops->push_back(loop);

  return INSP_OK;
}

insp_info insp_run (inspector_t* insp, int seed)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  insp->seed = seed;

  // aliases
  loop_list* loops = insp->loops;
  int tileSize = insp->tileSize;

  ASSERT((seed >= 0) && (seed < loops->size()), "Invalid tiling start point");
  loop_t* baseLoop = (*loops)[seed];

  // partition the iteration set of the base loop
  map_t* itset2tile = partition (baseLoop, tileSize);
  insp->itset2tile = itset2tile;

  return INSP_OK;
}

void insp_print (inspector_t* insp, insp_verbose level)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  // aliases
  loop_list* loops = insp->loops;
  map_t* itset2tile = insp->itset2tile;
  int seed = insp->seed;
  int tileSize = insp->tileSize;
  int itsetSize = (itset2tile) ? itset2tile->inSetSize : 0;
  int nTiles = (itset2tile) ? itset2tile->outSetSize : 0;
  int* itset2tileMap = (itset2tile) ? itset2tile->indMap : NULL;

  // set verbosity level
  int verbosity;
  switch (level) {
    case LOW: verbosity = MIN(LOW, itsetSize); break;
    case MEDIUM: verbosity = MIN(MEDIUM, itsetSize); break;
    case HIGH: verbosity = itsetSize;
  }

  cout << ":: Inspector info ::" << endl;
  if (loops) {
    cout << "Number of loops: " << loops->size() << ", base loop: " << seed << endl;
  }
  else {
    cout << "No loops specified" << endl;
  }
  cout << "Number of tiles: " << nTiles << endl;
  cout << "Tile size: " << tileSize << endl;
  if (itset2tile) {
    cout << endl << "Printing partioning of the base loop's iteration set:" << endl;
    cout << "  Iteration  |  tile "<< endl;
    for (int i = 0; i < verbosity; i++) {
      cout << "         " << i << "   |   " << itset2tileMap[i] << endl;
    }
    if (verbosity < itsetSize) {
      cout << "..." << endl;
      cout << "         " << itsetSize -1 << "   |   ";
      cout << itset2tileMap[itsetSize -1] << endl;
    }
  }
  else {
    cout << "No partitioning of the base loop performed" << endl;
  }
}

void insp_free (inspector_t* insp)
{
  free (insp->loops);
  free (insp);
}
