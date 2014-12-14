/*
 *  tile.c
 *
 */

#include "tile.h"
#include "utils.h"


tile_t* tile_init (int spannedLoops)
{
  tile_t* tile = (tile_t*) malloc (sizeof(tile_t));
  tile->iterations = (iterations_list**) malloc (sizeof(iterations_list*)*spannedLoops);
  for (int i = 0; i < spannedLoops; i++) {
    tile->iterations[i] = new iterations_list;
  }
  tile->spannedLoops = spannedLoops;
  return tile;
}

static inline void tile_add_iteration (tile_t* tile, int loopID, int iteration)
{
  tile->iterations[loopID]->push_back(iteration);
}

void tile_assign_loop (tile_list* tiles, int loopID, map_t* iter2tile)
{
  // aliases
  int itSetSize = iter2tile->inSet->size;
  int* iter2tileMap = iter2tile->indMap;

  for (int i = 0; i < itSetSize; i++) {
    tile_add_iteration ((*tiles)[iter2tileMap[i]], loopID, i);
  }
}

void tile_free (tile_t* tile)
{
  for (int i = 0; i < tile->spannedLoops; i++) {
    delete tile->iterations[i];
  }
  free (tile->iterations);
}
