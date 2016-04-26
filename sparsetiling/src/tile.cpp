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

void tile_assign_loop (tile_list* tiles, loop_t* loop, int* iter2tileMap)
{
  // aliases
  int loopIndex = loop->index;
  set_t* loopSet = loop->set;

  // 1) remove any previously assigned iteration for loop /loopIndex/
  tile_list::const_iterator tIt, tEnd;
  for (tIt = tiles->begin(), tEnd = tiles->end(); tIt != tEnd; tIt++) {
    (*tIt)->iterations[loopIndex]->clear();
  }

  // 2) distribute iterations to tiles (note: we do not assign non-exec iterations)
  int execSize = loopSet->core + loopSet->execHalo;
  for (int i = 0; i < execSize; i++) {
    tiles->at(iter2tileMap[i])->iterations[loopIndex]->push_back(i);
  }

  for (tIt = tiles->begin(), tEnd = tiles->end(); tIt != tEnd; tIt++) {
    iterations_list& iterations = *((*tIt)->iterations[loopIndex]);
    if (! iterations.size()) {
      continue;
    }

    // 3) sort the iterations within each tile, hopefully creating some spatial locality
    std::sort (iterations.begin(), iterations.end());

    // 4) add fake iterations in case one wants to start prefetching iteration /i+1/
    // while about to execute iteration /i/
    for (int i = 0; i < (*tIt)->prefetchHalo; i++) {
      iterations.push_back(iterations.back());
    }
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
