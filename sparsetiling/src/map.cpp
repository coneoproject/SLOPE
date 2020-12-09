/*
 * map.cpp
 *
 */

#include <algorithm>

#include <stdlib.h>

#include "map.h"
#include "utils.h"
#include "common.h"

#ifdef OP2

map_t* map (std::string name, set_t* inSet, set_t* outSet, int* values, int size, int dim, int mapBase)
{
  map_t* map = new map_t;

  map->name = name;
  map->inSet = inSet;
  map->outSet = outSet;
  map->values = values;
  map->size = size;
  map->offsets = NULL;
  map->dim = dim;
  map->mappedValues = NULL;
  map->mappedSize = 0;
  map->mapBase = mapBase;

  return map;
}

map_t* map_f (const char* name, set_t* inSet, set_t* outSet, int* values, int size, int dim, int mapBase)
{
  return map(std::string(name), inSet, outSet, values, size, dim, mapBase);
}

#else
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

map_t* map_f (const char* name, set_t* inSet, set_t* outSet, int* values, int size, int mapBase)
{
  if (mapBase == 1)
  {
    for (int i = 0; i < size; ++i)
    {
      values[i] = values[i] - 1;
    }
  }
  
  return map(std::string(name), inSet, outSet, values, size);
}

#endif

map_t* imap (std::string name, set_t* inSet, set_t* outSet, int* values, int* offsets)
{
  map_t* map = new map_t;

  map->name = name;
  map->inSet = inSet;
  map->outSet = outSet;
  map->values = values;
  map->size = offsets[inSet->size];
  map->offsets = offsets;
  map->dim = -2;

  return map;
}

map_t* map_cpy (std::string name, map_t* map)
{
  map_t* new_map = new map_t;

  new_map->name = name;
  new_map->inSet = set_cpy (map->inSet);
  new_map->outSet = set_cpy (map->outSet);

  new_map->values = new int[map->size];
  std::copy (map->values, map->values + map->size, new_map->values);
  new_map->size = map->size;

  if (map->offsets) {
    new_map->offsets = new int[map->inSet->size];
    std::copy (map->offsets, map->offsets + map->inSet->size, new_map->offsets);
  }
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
#ifdef OP2
    *size = (map->dim < 0) ? map->size / map->inSet->size : map->dim;
#else
    *size = map->size / map->inSet->size;
#endif
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
  int x2yArity = -1;
#ifdef OP2
  if(x2y->dim < 0){
    x2yArity = x2yMapSize / xSize;
  }else{
    x2yArity = x2y->dim;
  }
#else
  x2yArity = x2yMapSize / xSize;
#endif

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
  return imap ("inverse_" + x2y->name, set_cpy(x2y->outSet), set_cpy(x2y->inSet),
               y2xMap, y2xOffset);
}

// core | size | imp exec 0 | imp nonexec 0 | imp exec 1 | imp nonexec 1 |
void convert_map_vals_to_normal(map_t* map){

  set_t* outSet = map->outSet;
  int haloLevel = outSet->curHaloLevel;
  int* unmappedVals = new int[map->size];
  
  int start = 0;
  int end = 0;
  int index = 0;
  for(int hl = 0; hl < haloLevel; hl++){

    int execSize = map->inSet->execSizes[hl];
    int nonExecSize = 0;
    int prevNonExecSize = 0;
    if(hl > 0){
      for(int sl = 0; sl < hl; sl++){
        nonExecSize += map->inSet->nonExecSizes[sl];
      }
      prevNonExecSize = map->inSet->nonExecSizes[hl - 1];
    }

    start = end + prevNonExecSize * map->dim;
    end = (map->inSet->setSize + execSize + nonExecSize) * map->dim;
    
    for(int i = start; i < end; i++){
      // check for non exec values
      int nonExecOffset = 0;
      for(int j = 0; j < haloLevel - 1; j++){
        nonExecOffset += outSet->nonExecSizes[j];
      }
      int execOffset = outSet->execSizes[haloLevel - 1];
      int totalOffset = outSet->setSize + execOffset + nonExecOffset;
      int mapVal = map->values[i];
      if(mapVal > totalOffset - 1){
        unmappedVals[index++] = mapVal - nonExecOffset;
      }else{
      // check for exec values
        int execAvail = 0;
        for(int j = haloLevel - 1; j > 0; j--){
          nonExecOffset = 0;
          for(int k = 0; k < j; k ++){
            nonExecOffset += outSet->nonExecSizes[k];
          }
          execOffset = outSet->execSizes[j - 1];
          totalOffset = outSet->setSize + execOffset + nonExecOffset;
          if(mapVal > totalOffset - 1){
            unmappedVals[index++] = mapVal - nonExecOffset;
            execAvail = 1;
            break;
          }
        }
        if(execAvail == 0){
          unmappedVals[index++] = mapVal;
        }
      }
    }
  }

  // int rank;
  // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  //   if(rank == 0){
    // printf("map->values map=%s size=%d out=%s size=%d exec=%d,%d nonexec=%d,%d\n", map->name.c_str(), map->size, outSet->name.c_str(), outSet->setSize, outSet->execSizes[0], outSet->execSizes[1], outSet->nonExecSizes[0], outSet->nonExecSizes[1]);
    // printf("map->values map=%s size=%d\n", map->name.c_str(), map->size);
    // PRINT_INTARR(map->values, 0, map->size);
    // printf("unmappedVals map=%s size=%d\n", map->name.c_str(), map->size);
    // PRINT_INTARR(unmappedVals, 0, map->size);
  // }

  map->mappedValues = new int[map->size];
  memcpy(map->mappedValues, map->values, map->size * sizeof(int));
  // delete[] map->values;  // cannot delete this, since this is the copy of op2
  map->values = NULL;
  map->values = unmappedVals;
}

map_list* map_list_f()
{
    return new map_list;
}

void insert_map_to_f(map_list* maps, map_t* map)
{
    maps->insert(map);
}
