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
  int reminderTileSize = setSize % nParts;
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

  // compute offsets to create an irregular map since the last tile won't have
  // same size as the other tiles if the iteration set size is not a multiple
  // of the specified tile size
  int* offsets = new int[nTiles + 1];
  offsets[0] = 0;
  for (i = 1; i < nTiles; i++) {
    offsets[i] = tileSize + offsets[i - 1];
  }
  offsets[nTiles] = setSize;

  return imap ("iter2t", set_cpy(loop->set), set("tiles", nTiles), indMap, offsets);
}
