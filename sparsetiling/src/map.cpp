/*
 * map.cpp
 *
 */

#include <stdlib.h>

#include "map.h"
#include "utils.h"

map_t* map (set_t* inSet, set_t* outSet, int* indMap, int mapSize)
{
  map_t* map = (map_t*) malloc (sizeof(map_t));
  map->inSet = inSet;
  map->outSet = outSet;
  map->indMap = indMap;
  map->mapSize = mapSize;
  map->offsets = NULL;
  return map;
}

map_t* imap (set_t* inSet, set_t* outSet, int* indMap, int* offsets)
{
  map_t* map = (map_t*) malloc (sizeof(map_t));
  map->inSet = inSet;
  map->outSet = outSet;
  map->indMap = indMap;
  map->mapSize = -1;
  map->offsets = offsets;
  return map;
}

void map_free (map_t* map)
{
  set_free (map->inSet);
  set_free (map->outSet);
  free (map->offsets);
  free (map);
}

map_t* map_invert (map_t* x2y, int xOffset, int* maxIncidence)
{
  // aliases
  int xSize = x2y->inSet->size;
  std::string xName = x2y->inSet->setName;
  int ySize = x2y->outSet->size;
  std::string yName = x2y->outSet->setName;
  int* x2yMap = x2y->indMap;
  int x2yMapSize = x2y->mapSize;

  int x2yAriety = x2yMapSize / xSize;

  int* y2xMap = (int*) malloc (sizeof(int)*x2yMapSize);
  int* y2xOffset = (int*) calloc (ySize + 1, sizeof(int));
  int incidence = 0;

  // compute the offsets in y2x
  y2xOffset[0] = 0;
  for (int i = 0; i < x2yMapSize; i++) {
    y2xOffset[x2yMap[i] + xOffset]++;
  }
  for (int i = 1; i < ySize; i++) {
    y2xOffset[i + xOffset] += y2xOffset[i - 1 + xOffset];
    incidence = MAX(incidence, y2xOffset[i + xOffset] - y2xOffset[i - 1 + xOffset]);
  }

  // compute y2x
  int* inserted = (int*) calloc (ySize + 1, sizeof(int));
  for (int i = 0; i < x2yMapSize; i++) {
    for (int j = 0; j < x2yAriety; j++) {
      int entry = x2yMap[i*x2yAriety + j] - 1 + xOffset;
      y2xMap[y2xOffset[entry] + inserted[entry]] = i / x2yAriety;
      inserted[entry]++;
    }
  }
  free (inserted);

  if (maxIncidence)
    *maxIncidence = incidence;
  return imap (set(xName, xSize), set(yName, ySize), y2xMap, y2xOffset);
}
