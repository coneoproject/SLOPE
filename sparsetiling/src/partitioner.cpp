/*
 *  partitioner.cpp
 *
 * Implement routine(s) for partitioning iteration sets
 */

#include <omp.h>

#include "partitioner.h"

std::pair<map_t*, tile_list*> partition (inspector_t* insp, loop_t* loop, int tileSize)
{
  // aliases
  int setCore = loop->set->core;
  int setExecHalo = loop->set->execHalo;
  int setNonExecHalo = loop->set->nonExecHalo;
  int setSize = loop->set->size;
  int nLoops = insp->loops->size();

  // prepare for partitioning
  int i;
  int nParts, remainderTileSize;
  int tileID = -1;
  int* indMap = new int[setSize];

  // partition the local core region
  nParts = setCore / tileSize;
  remainderTileSize = setCore % tileSize;
  int nCoreTiles = nParts + ((remainderTileSize > 0) ? 1 : 0);
  for (i = 0; i < setCore - remainderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[i] = tileID;
  }
  tileID = (remainderTileSize > 0) ? tileID + 1 : tileID;
  for (; i < setCore; i++) {
    indMap[i] = tileID;
  }

  // partition the exec halo region
  // this region is expected to be smaller than core, so we shrunk /tileSize/
  // accordingly to have enough parallelism
  int nThreads = omp_get_max_threads();
  if (setExecHalo <= nThreads) {
    tileSize = setExecHalo;
  }
  else if (setExecHalo > nThreads*2) {
    tileSize = setExecHalo / (nThreads*2);
  }
  else {
    // nThreads < setExecHalo < nThreads*2
    tileSize = setExecHalo / nThreads;
  }
  // now do the actual partitioning
  nParts = (setExecHalo > 0) ? setExecHalo / tileSize : 0;
  remainderTileSize = (setExecHalo > 0) ? setExecHalo % tileSize : 0;
  int nExecHaloTiles = nParts + ((remainderTileSize > 0) ? 1 : 0);
  for (i = 0; i < setExecHalo - remainderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[setCore + i] = tileID;
  }
  tileID = (remainderTileSize > 0) ? tileID + 1 : tileID;
  for (; i < setExecHalo; i++) {
    indMap[setCore + i] = tileID;
  }

  // partition the non-exec halo region
  // this is never going to be executed, so a single tile is fine
  int nNonExecHaloTiles = 0;
  if (setNonExecHalo > 0) {
    nNonExecHaloTiles = 1;
    tileID++;
    for (; i < setSize; i++) {
      indMap[i] = tileID;
    }
  }

  // initialize tiles
  int t;
  tile_list* tiles = new tile_list (nCoreTiles + nExecHaloTiles + nNonExecHaloTiles);
  for (t = 0; t < nCoreTiles; t++) {
    tiles->at(t) = tile_init (nLoops);
  }
  for (; t < nCoreTiles + nExecHaloTiles; t++) {
    tiles->at(t) = tile_init (nLoops, EXEC_HALO);
  }
  for (; t < nCoreTiles + nExecHaloTiles + nNonExecHaloTiles; t++) {
    tiles->at(t) = tile_init (nLoops, NON_EXEC_HALO);
  }

  // track tiles, now logically split over core, exec_halo, and non_exec_halo regions
  set_t* tileRegions = set("tiles", nCoreTiles, nExecHaloTiles, nNonExecHaloTiles);
  insp->tileRegions = tileRegions;

  // bind loop iterations to tiles
  map_t* iter2tile = map ("i2t", set_cpy(loop->set), set_cpy(tileRegions), indMap, setSize);

  return std::make_pair(iter2tile, tiles);
}
