/*
 * map.cpp
 *
 */

#include <stdlib.h>

#include "map.h"
#include "utils.h"

map_t* map (std::string name, set_t* inSet, set_t* outSet, int* values, int size)
{
  map_t* map = new map_t;
  map->name = name;
  map->inSet = inSet;
  map->outSet = outSet;
  map->values = values;
  map->size = size;
  map->offsets = NULL;
  return map;
}

map_t* imap (std::string name, set_t* inSet, set_t* outSet, int* values, int* offsets)
{
  map_t* map = new map_t;
  map->name = name;
  map->inSet = inSet;
  map->outSet = outSet;
  map->values = values;
  map->size = offsets[inSet->size];
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
    delete[] map->values;
  }
  delete map;
}

map_t* map_invert (map_t* x2y, int* maxIncidence)
{
  // aliases
  int xSize = x2y->inSet->size;
  int ySize = x2y->outSet->size;
  int* x2yMap = x2y->values;
  int x2yMapSize = x2y->size;

  int x2yAriety = x2yMapSize / xSize;

  int* y2xMap = new int[x2yMapSize];
  int* y2xOffset = new int[ySize + 1]();
  int incidence = 0;

  // compute the offsets in y2x
  for (int i = 0; i < x2yMapSize; i++) {
    y2xOffset[x2yMap[i] + 1]++;
  }
  for (int i = 1; i < ySize + 1; i++) {
    y2xOffset[i] += y2xOffset[i - 1];
    incidence = MAX(incidence, y2xOffset[i] - y2xOffset[i - 1]);
  }

  // compute y2x
  int* inserted = new int[ySize + 1]();
  for (int i = 0; i < x2yMapSize; i += x2yAriety) {
    for (int j = 0; j < x2yAriety; j++) {
      int entry = x2yMap[i + j];
      y2xMap[y2xOffset[entry] + inserted[entry]] = i / x2yAriety;
      inserted[entry]++;
    }
  }
  delete[] inserted;

  if (maxIncidence)
    *maxIncidence = incidence;
  return imap ("inverted_" + x2y->name, set_cpy(x2y->outSet), set_cpy(x2y->inSet),
               y2xMap, y2xOffset);
}
