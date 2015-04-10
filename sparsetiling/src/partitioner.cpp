/*
 *  partitioner.cpp
 *
 * Implement routine(s) for partitioning iteration sets
 */

#include "partitioner.h"

std::pair<map_t*, tile_list*> partition (inspector_t* insp, loop_t* loop, int tileSize)
{
  // aliases
  int setCore = loop->set->core;
  int setExecHalo = loop->set->execHalo;
  int setNonExecHalo = loop->set->nonExecHalo;
  int setSize = loop->set->size;
  int nLoops = insp->loops->size();

  int* indMap = new int[setSize];

  // tile the local iteration space
  int nParts = setCore / tileSize;
  int remainderTileSize = setCore % tileSize;
  int nCoreTiles = nParts + ((remainderTileSize > 0) ? 1 : 0);
  int tileID = -1;
  int i = 0;
  for (; i < setCore - remainderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[i] = tileID;
  }
  tileID = (remainderTileSize > 0) ? tileID + 1 : tileID;
  for (; i < setCore; i++) {
    indMap[i] = tileID;
  }

  // create the list of tiles
  tile_list* tiles = new tile_list (nCoreTiles);
  for (int t = 0; t < nCoreTiles; t++) {
    tiles->at(t) = tile_init (nLoops);
  }

  // tile the halo region. We create a single tile spanning /execHalo/ as well as
  // a single tile spanning /nonExecHalo/
  int nExecHaloTiles = 0, nNonExecHaloTiles = 0;
  if (setExecHalo > 0) {
    tileID++; nExecHaloTiles++;
    for (; i < setCore + setExecHalo; i++) {
      indMap[i] = tileID;
    }
    tiles->push_back (tile_init(nLoops, EXEC_HALO));
  }
  if (setNonExecHalo > 0) {
    tileID++; nNonExecHaloTiles++;
    for (; i < setSize; i++) {
      indMap[i] = tileID;
    }
    tiles->push_back (tile_init(nLoops, NON_EXEC_HALO));
  }

  // track tiles now logically split over core, exec_halo, and non_exec_halo regions
  set_t* tileRegions = set("tiles", nCoreTiles, nExecHaloTiles, nNonExecHaloTiles);
  insp->tileRegions = tileRegions;

  // bind loop iterations to tiles
  map_t* iter2tile = map ("i2t", set_cpy(loop->set), set_cpy(tileRegions), indMap, setSize);

  return std::make_pair(iter2tile, tiles);
}
