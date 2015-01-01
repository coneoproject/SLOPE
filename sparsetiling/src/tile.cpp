/*
 *  tile.c
 *
 */

#include "tile.h"

tile_t* tile_init (int crossedLoops)
{
  tile_t* tile = new tile_t;
  tile->iterations = new iterations_list*[crossedLoops];
  for (int i = 0; i < crossedLoops; i++) {
    tile->iterations[i] = new iterations_list;
  }
  tile->localMaps = new mapname_iterations*[crossedLoops];
  tile->crossedLoops = crossedLoops;
  tile->color = -1;
  return tile;
}

void tile_assign_loop (tile_list* tiles, int loopIndex, int itSetSize, int* iter2tileMap)
{
  for (int i = 0; i < itSetSize; i++) {
    tiles->at(iter2tileMap[i])->iterations[loopIndex]->push_back(i);
  }
}

void tile_free (tile_t* tile)
{
  for (int i = 0; i < tile->crossedLoops; i++) {
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
