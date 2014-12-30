/*
 *  tile.c
 *
 */

#include "tile.h"

tile_t* tile_init (int spannedLoops)
{
  tile_t* tile = new tile_t;
  tile->iterations = new iterations_list*[spannedLoops];
  for (int i = 0; i < spannedLoops; i++) {
    tile->iterations[i] = new iterations_list;
  }
  tile->localMaps = new mapname_iterations*[spannedLoops];
  tile->spannedLoops = spannedLoops;
  tile->color = -1;
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
    // delete loop's iterations belonging to tile
    delete tile->iterations[i];
    // delete loop's local maps
    mapname_iterations* localMap = tile->localMaps[i];
    mapname_iterations::iterator it, end;
    for (it = localMap->begin(), end = localMap->end(); it != end; it++) {
      delete it->second;
    }
    delete localMap;
  }
  delete[] tile->iterations;
  delete[] tile->localMaps;
  delete tile;
}
