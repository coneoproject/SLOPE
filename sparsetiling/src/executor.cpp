/*
 *  executor.cpp
 *
 */

#include "executor.h"
#include <iostream>

executor_t* exec_init (inspector_t* insp)
{
  // aliases
  tile_list* tiles = insp->tiles;
  int nTiles = tiles->size();
  set_t* tile_set = set_cpy (insp->iter2tile->outSet);
  set_t* color_set = set_cpy (insp->iter2color->outSet);

  executor_t* exec = new executor_t;

  // compute a map from colors to tiles IDs
  int* tile2colorIndMap = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    tile2colorIndMap[i] = tiles->at(i)->color;
  }
  map_t* tile2color = map (tile_set, color_set, tile2colorIndMap, nTiles);

  exec->tiles = tiles;
  exec->color2tile = map_invert (tile2color, NULL);

  map_free (tile2color, true);

  return exec;
}

void exec_free (executor_t* exec)
{
  for (int i = 0; i < exec->tiles->size(); i++) {
    tile_free (exec->tiles->at(i));
  }
  delete exec->tiles;
  map_free (exec->color2tile, true);
  delete exec;
}
