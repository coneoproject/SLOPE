/*
 *  tiling.cpp
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#include "tiling.h"
#include "utils.h"

iter2tc_t* iter2tc_init (int itSetSize, int* iter2tile, int* iter2color)
{
  iter2tc_t* iter2tc = (iter2tc_t*) malloc (sizeof(iter2tc_t));

  iter2tc->itSetSize = itSetSize;
  iter2tc->iter2tile = iter2tile;
  iter2tc->iter2color = iter2color;

  return iter2tc;
}

void iter2tc_free (iter2tc_t* iter2tc)
{
  free (iter2tc->iter2tile);
  free (iter2tc->iter2color);
  free (iter2tc);
}

projection_t* project_forward (loop_t* tiledLoop, iter2tc_t* tilingInfo)
{
  // aliases
  desc_list* descriptors = tiledLoop->descriptors;
  int* iter2tile = tilingInfo->iter2tile;
  int* iter2color = tilingInfo->iter2color;

  projection_t* projection = (projection_t*) malloc (sizeof(projection_t));

desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    // aliases
    map_t* descMap = (*it)->map;
    am_t descMode = (*it)->mode;

    iter2tc_t* descIter2tc;
    if (descMap == DIRECT) {
      // direct set case: just replicate the current tiling
      // note that multiple DIRECT descriptors have no effect on the projection,
      // since a requirement for a projection is that its elements are unique
      descIter2tc = tilingInfo;
    }
    else {
      // indirect set case
      // aliases
      int inSetSize = descMap->inSet->size;
      int outSetSize = descMap->outSet->size;
      int mapSize = descMap->mapSize;
      int* indMap = descMap->indMap;

      int* descIter2tile = (int*) malloc (sizeof(int)*outSetSize);
      int* descIter2color = (int*) malloc (sizeof(int)*outSetSize);
      std::fill_n (descIter2tile, outSetSize, -1);
      std::fill_n (descIter2color, outSetSize, -1);

      descIter2tc = iter2tc_init (outSetSize, descIter2tile, descIter2color);

      // iterate over the tiledLoop's iteration set, and use the map to access
      // the touched adjacent entities.
      for (int i = 0; i < inSetSize; i++) {
        int iterTile = iter2tile[i];
        int iterColor = iter2color[i];
        for (int j = 0; j < mapSize; j++) {
          int indIter = indMap[i*mapSize + j];
          int indColor = MAX(iterColor, descIter2color[indIter]);
          if (indColor != descIter2color[indIter]) {
            // update color and tile of the indirect touched iteration
            descIter2color[indIter] = indColor;
            descIter2tile[indIter] = iterTile;
          }
        }
      }
    }

    projection->insert(*descIter2tc);
  }

  return projection;
}

projection_t* project_backward (loop_t* tiledLoop, iter2tc_t tilingInfo)
{
  return NULL;
}

void tile_forward ()
{

}

void tile_backward ()
{

}
