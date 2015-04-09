/*
 *  partitioner.cpp
 *
 * Implement routine(s) for partitioning iteration sets
 */

#include "partitioner.h"

map_t* partition (loop_t* loop, int tileSize)
{
  // aliases
  int setSize = loop->set->size;
  int setExecHalo = loop->set->execHalo;
  int setNonExecHalo = loop->set->nonExecHalo;

  int setTotalSize = setSize + setExecHalo + setNonExecHalo;
  int* indMap = new int[setTotalSize];

  // tile the local iteration space
  int nParts = setSize / tileSize;
  int reminderTileSize = setSize % tileSize;
  int nTiles = nParts + ((reminderTileSize > 0) ? 1 : 0);
  int tileID = -1;
  int i = 0;
  for (; i < setSize - reminderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[i] = tileID;
  }
  tileID++;
  for (; i < setSize; i++) {
    indMap[i] = tileID;
  }

  // tile the halo region. We create a single tile spanning /execHalo/ as well as
  // a single tile spanning /nonExecHalo/
  tileID++;
  for (; i < setExecHalo; i++) {
    indMap[i] = tileID;
  }
  nTiles = nTiles + ((setExecHalo > 0) ? 1 : 0);
  tileID++;
  for (; i < setNonExecHalo; i++) {
    indMap[i] = tileID;
  }
  nTiles = nTiles + ((setExecHalo > 0) ? 1 : 0);

  return map ("i2t", set_cpy(loop->set), set("tiles", nTiles), indMap, setTotalSize*1);
}
