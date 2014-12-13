/*
 *  tiling.cpp
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#include "tiling.h"

iter2tc_t* make_iter2tc (map_t* iter2tile, map_t* iter2color)
{
  iter2tc_t* iter2tc = (iter2tc_t*) malloc (sizeof(iter2tc_t));

  iter2tc->itsetSize = iter2tile->inSet->size;
  iter2tc->iter2tile = iter2tile->indMap;
  iter2tc->nTiles = iter2tile->outSet->size;
  iter2tc->offsets = iter2tile->offsets;
  iter2tc->iter2color = iter2color->indMap;
  iter2tc->nColors = iter2color->outSet->size;

  // Note that free is called instead of map_free because the content of the maps
  // now belongs to iter2tc
  free (iter2tile);
  free (iter2color);

  return iter2tc;
}

void iter2tc_free (iter2tc_t* iter2tc)
{
  free (iter2tc->iter2tile);
  free (iter2tc->iter2color);
  free (iter2tc->offsets);
  free (iter2tc);
}

projection_list* project_forward (loop_t* tiledLoop, iter2tc_t tilingInfo)
{
  return NULL;
}

projection_list* project_backward (loop_t* tiledLoop, iter2tc_t tilingInfo)
{
  return NULL;
}

void tile_forward ()
{

}

void tile_backward ()
{

}
