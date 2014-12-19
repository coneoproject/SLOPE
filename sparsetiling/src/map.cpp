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

void map_invert (map_t* x2y, int xZero, map_t* y2x, int* maxIncidence)
{
}
