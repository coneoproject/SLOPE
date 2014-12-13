/*
 *  inspector.cpp
 *
 * Implement inspector routines
 */

#include <stdlib.h>

#include "inspector.h"
#include "utils.h"
#include "partitioner.h"
#include "coloring.h"

using namespace std;

inspector_t* insp_init (int tileSize, insp_strategy strategy)
{
  inspector_t* insp = (inspector_t*) malloc (sizeof(inspector_t));

  insp->strategy = strategy;
  insp->tileSize = tileSize;
  insp->loops = (loop_list*) malloc (sizeof(loop_list*));

  insp->seed = -1;
  insp->iter2tile = NULL;

  return insp;
}

insp_info insp_add_parloop (inspector_t* insp, char* loopName, set_t* set,
                            desc_list* descriptors)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  loop_t* loop = (loop_t*) malloc (sizeof(loop_t));
  loop->loopName = loopName;
  loop->set = set;
  loop->descriptors = descriptors;

  insp->loops->push_back(loop);

  return INSP_OK;
}

insp_info insp_run (inspector_t* insp, int seed)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  insp->seed = seed;

  // aliases
  insp_strategy strategy = insp->strategy;
  loop_list* loops = insp->loops;
  loop_t* baseLoop = (*loops)[seed];
  int tileSize = insp->tileSize;

  ASSERT((seed >= 0) && (seed < loops->size()), "Invalid tiling start point");

  // partition the iteration set of the base loop and create empty tiles
  map_t* iter2tile = partition (baseLoop, tileSize);
  int nTiles = iter2tile->outSet->size;
  tile_list* tiles = new tile_list (nTiles);
  for (int i = 0; i < nTiles; i++) {
    (*tiles)[i] = tile_init (loops->size());
  }
  tile_assign_loop (tiles, seed, iter2tile);

  // track information essential for the executor
  insp->iter2tile = iter2tile;
  insp->tiles = tiles;

  // color the base loop's sets
  switch (strategy) {
    case SEQUENTIAL: case MPI:
      color_sequential (baseLoop);
      break;
    case OMP: case OMP_MPI:
      color_kdistance (baseLoop);
      break;
  }

  return INSP_OK;
}

void insp_print (inspector_t* insp, insp_verbose level)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  // aliases
  loop_list* loops = insp->loops;
  map_t* iter2tile = insp->iter2tile;
  int seed = insp->seed;
  int tileSize = insp->tileSize;
  int itsetSize = (iter2tile) ? iter2tile->inSet->size : 0;
  int nTiles = (iter2tile) ? iter2tile->outSet->size : 0;
  int* iter2tileMap = (iter2tile) ? iter2tile->indMap : NULL;
  tile_list* tiles = insp->tiles;

  // set verbosity level
  int verbosityItSet, verbosityTiles;;
  switch (level) {
    case LOW:
      verbosityItSet = MIN(LOW, itsetSize);
      verbosityTiles = nTiles / 2;
      break;
    case MEDIUM:
      verbosityItSet = MIN(MEDIUM, itsetSize);
      verbosityTiles = nTiles;
      break;
    case HIGH:
      verbosityItSet = itsetSize;
      verbosityTiles = nTiles;
  }

  cout << endl << ":: Inspector info ::" << endl << endl;
  if (loops) {
    cout << "Number of loops: " << loops->size() << ", base loop: " << seed << endl;
  }
  else {
    cout << "No loops specified" << endl;
  }
  cout << "Number of tiles: " << nTiles << endl;
  cout << "Tile size: " << tileSize << endl;
  if (iter2tile) {
    cout << endl << "Printing partioning of the base loop's iteration set:" << endl;
    cout << "  Iteration  |  Tile "<< endl;
    for (int i = 0; i < verbosityItSet; i++) {
      cout << "         " << i << "   |   " << iter2tileMap[i] << endl;
    }
    if (verbosityItSet < itsetSize) {
      cout << "..." << endl;
      cout << "         " << itsetSize -1 << "   |   ";
      cout << iter2tileMap[itsetSize -1] << endl;
    }
  }
  else {
    cout << "No partitioning of the base loop performed" << endl;
  }

  if (tiles) {
    cout << endl << "Printing tiles' base loop iterations" << endl;
    cout << "       Tile  |  Iteration "<< endl;
    for (int i = 0; i < verbosityTiles; i++) {
      int tileSize = (*tiles)[i]->iterations[seed]->size();
      for (int j = 0; j < tileSize; j++) {
        cout << "         " << i;
        cout << "   |   " << (*tiles)[i]->iterations[seed]->at(j) << endl;
      }
    }
  }
}

void insp_free (inspector_t* insp)
{
  for (int i = 0; i < insp->tiles->size(); i++) {
    free ((*insp->tiles)[i]);
  }
  delete insp->tiles;
  free (insp->loops);
  free (insp);
}
