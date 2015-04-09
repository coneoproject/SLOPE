/*
 *  partitioner.cpp
 *
 * Implement routine(s) for partitioning iteration sets
 */

#include "partitioner.h"

std::pair<map_t*, tile_list*> partition (loop_t* loop, int tileSize, int crossedLoops)
{
  // aliases
  int setCore = loop->set->core;
  int setExecHalo = loop->set->execHalo;
  int setNonExecHalo = loop->set->nonExecHalo;
  int setSize = loop->set->size;

  int* indMap = new int[setSize];

  // tile the local iteration space
  int nParts = setCore / tileSize;
  int reminderTileSize = setCore % tileSize;
  int nTiles = nParts + ((reminderTileSize > 0) ? 1 : 0);
  int tileID = -1;
  int i = 0;
  for (; i < setCore - reminderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[i] = tileID;
  }
  tileID++;
  for (; i < setCore; i++) {
    indMap[i] = tileID;
  }

  // create the list of tiles
  tile_list* tiles = new tile_list (nTiles);
  int t = 0;
  for (; t < nTiles; t++) {
    tiles->at(t) = tile_init (crossedLoops);
  }

  // tile the halo region. We create a single tile spanning /execHalo/ as well as
  // a single tile spanning /nonExecHalo/
  if (setExecHalo > 0) {
    tileID++; nTiles++;
    for (; i < setCore + setExecHalo; i++) {
      indMap[i] = tileID;
    }
    tiles->push_back (tile_init(crossedLoops, EXEC_HALO));
  }
  if (setNonExecHalo > 0) {
    tileID++; nTiles++;
    for (; i < setSize; i++) {
      indMap[i] = tileID;
    }
    tiles->push_back (tile_init(crossedLoops, NON_EXEC_HALO));
  }

  // bind loop iterations to tiles
  map_t* iter2tile = map ("i2t", set_cpy(loop->set), set("tiles", nTiles), indMap, setSize);

  return std::make_pair(iter2tile, tiles);
}
