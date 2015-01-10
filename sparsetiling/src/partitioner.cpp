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

  int* indMap = new int[setSize];

  int nParts = setSize / tileSize;
  int reminderTileSize = setSize % tileSize;
  int nTiles = nParts + ((reminderTileSize > 0) ? 1 : 0);

  int tileID = -1;
  int i;
  for (i = 0; i < setSize - reminderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[i] = tileID;
  }
  tileID++;
  for (; i < setSize; i++) {
    indMap[i] = tileID;
  }

  return map ("i2t", set_cpy(loop->set), set("tiles", nTiles), indMap, setSize*1);
}
