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

  int* indMap = (int*) malloc (sizeof(int)*setSize);

  int nParts = setSize / tileSize;
  int reminderTileSize = setSize % nParts;
  int nTiles = nParts + ((reminderTileSize > 0) ? 1 : 0);

  int partID = -1;
  int i;
  for (i = 0; i < setSize - reminderTileSize; i++) {
    partID = (i % tileSize == 0) ? partID + 1 : partID;
    indMap[i] = partID;
  }
  for (; i < setSize; i++) {
    indMap[i] = partID;
  }

  // compute offsets to create an irregular map since the last tile won't have
  // same size as the other tiles if the iteration set size is not a multiple
  // of the specified tile size
  int* offsets = (int*) malloc (sizeof(int)*(nTiles + 1));
  offsets[0] = 0;
  for (i = 1; i < nTiles; i++) {
    offsets[i] = tileSize + offsets[i - 1];
  }
  offsets[nTiles] = setSize;

  set_t* tileSet = set ("tiles", nTiles);
  return imap (loop->set, tileSet, indMap, offsets);
}
