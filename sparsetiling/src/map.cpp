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

void map_ofs (map_t* map, int element, int* offset, int* size)
{
  ASSERT(element < map->inSet->size, "Invalid element passed to map_ofs");

  if (map->offsets) {
    // imap case
    *size = map->offsets[element + 1] - map->offsets[element];
    *offset = map->offsets[element];
  }
  else {
    // size == mapArity
    *size = map->size / map->inSet->size;
    *offset = element*(*size);
  }
}

map_t* map_invert (map_t* x2y, int* maxIncidence)
{
  // aliases
  int xSize = x2y->inSet->size;
  int ySize = x2y->outSet->size;
  int* x2yMap = x2y->values;
  int x2yMapSize = x2y->size;

  int x2yArity = x2yMapSize / xSize;

  int* y2xMap = new int[x2yMapSize];
  int* y2xOffset = new int[ySize + 1]();
  int incidence = 0;

  // compute the offsets in y2x
  // note: some entries in /x2yMap/ might be set to -1 to indicate that an element
  // in /x/ is on the boundary, and it is touching some off-processor elements in /y/;
  // so here we have to reset /y2xOffset[0]/ to 0
  for (int i = 0; i < x2yMapSize; i++) {
    y2xOffset[x2yMap[i] + 1]++;
  }
  y2xOffset[0] = 0;
  for (int i = 1; i < ySize + 1; i++) {
    y2xOffset[i] += y2xOffset[i - 1];
    incidence = MAX(incidence, y2xOffset[i] - y2xOffset[i - 1]);
  }

  // compute y2x
  int* inserted = new int[ySize + 1]();
  for (int i = 0; i < x2yMapSize; i += x2yArity) {
    for (int j = 0; j < x2yArity; j++) {
      int entry = x2yMap[i + j];
      if (entry == -1) {
        // as explained before, off-processor elements are ignored. In the end,
        // /y2xMap/ might just be slightly larger than strictly necessary
        continue;
      }
      y2xMap[y2xOffset[entry] + inserted[entry]] = i / x2yArity;
      inserted[entry]++;
    }
  }
  delete[] inserted;

  if (maxIncidence)
    *maxIncidence = incidence;
  return imap ("inverted_" + x2y->name, set_cpy(x2y->outSet), set_cpy(x2y->inSet),
               y2xMap, y2xOffset);
}
