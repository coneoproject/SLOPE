/*
 *  partitioner.cpp
 *
 * Implement a number of routines for partitioning iteration sets
 */

#include "partitioner.h"

map_t* partition (loop_t* loop, int tileSize)
{
  // aliases
  int setSize = loop->setSize;

  int* indMap = (int*) malloc (sizeof(int)*setSize);
  map_t* itset2tile = map_init (setSize, setSize, indMap, setSize);

  int nParts = setSize / tileSize;
  int reminderTileSize = setSize % nParts;
  int partID = -1;
  int i;
  for (i = 0; i < setSize - reminderTileSize; i++)
  {
    partID = (i % tileSize == 0) ? partID + 1 : partID;
    indMap[i] = partID;
  }
  for (; i < setSize; i++)
    indMap[i] = partID;

  return itset2tile;
}
