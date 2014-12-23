/*
 * map.cpp
 *
 */

#include <stdlib.h>

#include "map.h"
#include "utils.h"

map_t* map (set_t* inSet, set_t* outSet, int* indMap, int mapSize)
{
  map_t* map = new map_t;
  map->inSet = inSet;
  map->outSet = outSet;
  map->indMap = indMap;
  map->mapSize = mapSize;
  map->offsets = NULL;
  return map;
}

map_t* imap (set_t* inSet, set_t* outSet, int* indMap, int* offsets)
{
  map_t* map = new map_t;
  map->inSet = inSet;
  map->outSet = outSet;
  map->indMap = indMap;
  map->mapSize = -1;
  map->offsets = offsets;
  return map;
}

void map_free (map_t* map, bool freeIndMap)
{
  if (! map) {
    return;
  }

  set_free (map->inSet);
  set_free (map->outSet);
  delete[] map->offsets;
  if (freeIndMap) {
    delete[] map->indMap;
  }
  delete map;
}

map_t* map_invert (map_t* x2y, int xOffset, int* maxIncidence)
{
  // aliases
  int xSize = x2y->inSet->size;
  int ySize = x2y->outSet->size;
  int* x2yMap = x2y->indMap;
  int x2yMapSize = x2y->mapSize;

  int x2yAriety = x2yMapSize / xSize;

  int* y2xMap = new int[x2yMapSize];
  int* y2xOffset = new int[ySize + 1]();
  int incidence = 0;

  // compute the offsets in y2x
  for (int i = 0; i < x2yMapSize; i++) {
    y2xOffset[x2yMap[i] + xOffset]++;
  }
  for (int i = 1; i < ySize + 1; i++) {
    y2xOffset[i + xOffset] += y2xOffset[i - 1 + xOffset];
    incidence = MAX(incidence, y2xOffset[i + xOffset] - y2xOffset[i - 1 + xOffset]);
  }

  // compute y2x
  int* inserted = new int[ySize + 1]();
  for (int i = 0; i < x2yMapSize; i += x2yAriety) {
    for (int j = 0; j < x2yAriety; j++) {
      int entry = x2yMap[i + j] - 1 + xOffset;
      y2xMap[y2xOffset[entry] + inserted[entry]] = i / x2yAriety;
      inserted[entry]++;
    }
  }
  delete[] inserted;

  if (maxIncidence)
    *maxIncidence = incidence;
  return imap (set_cpy(x2y->inSet), set_cpy(x2y->outSet), y2xMap, y2xOffset);
}
