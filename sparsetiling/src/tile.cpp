/*
 *  tile.c
 *
 */

#include "tile.h"
#include "utils.h"

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
  // first, remove any previously assigned iteration for loop loopIndex
  tile_list::const_iterator tIt, tEnd;
  for (tIt = tiles->begin(), tEnd = tiles->end(); tIt != tEnd; tIt++) {
    (*tIt)->iterations[loopIndex]->clear();
  }

  // then, distribute iterations among the tiles
  for (int i = 0; i < itSetSize; i++) {
    tiles->at(iter2tileMap[i])->iterations[loopIndex]->push_back(i);
  }
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
