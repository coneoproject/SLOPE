/*
 *  tile.c
 *
 */

#include <algorithm>

#include "tile.h"
#include "utils.h"

tile_t* tile_init (int crossedLoops, tile_region region, int prefetchHalo)
{
  tile_t* tile = new tile_t;
  tile->iterations = new iterations_list*[crossedLoops];
  for (int i = 0; i < crossedLoops; i++) {
    tile->iterations[i] = new iterations_list;
  }
  tile->localMaps = new mapname_iterations*[crossedLoops];
  tile->crossedLoops = crossedLoops;
  tile->region = region;
  tile->color = -1;
  tile->prefetchHalo = prefetchHalo;
  return tile;
}

iterations_list& tile_get_local_map (tile_t* tile, int loopIndex, std::string mapName)
{
  ASSERT((loopIndex >= 0) && (loopIndex < tile->crossedLoops),
         "Invalid loop index while retrieving a local map");
  return *(tile->localMaps[loopIndex]->find(mapName)->second);
}

iterations_list& tile_get_iterations (tile_t* tile, int loopIndex)
{
  return *(tile->iterations[loopIndex]);
}

int tile_loop_size (tile_t* tile, int loopIndex)
{
  return tile->iterations[loopIndex]->size() - tile->prefetchHalo;
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
