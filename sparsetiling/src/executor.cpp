/*
 *  executor.cpp
 *
 */

#include "executor.h"
#include "utils.h"

executor_t* exec_init (inspector_t* insp)
{
  // aliases
  tile_list* tiles = insp->tiles;
  int nTiles = tiles->size();
  set_t* tileSet = set_cpy (insp->iter2tile->outSet);
  set_t* colorSet = set_cpy (insp->iter2color->outSet);

  executor_t* exec = new executor_t;

  // compute a map from colors to tiles IDs
  int* tile2colorIndMap = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    tile2colorIndMap[i] = tiles->at(i)->color;
  }
  map_t* tile2color = map ("t2c", tileSet, colorSet, tile2colorIndMap, nTiles);

  exec->tiles = tiles;
  exec->color2tile = map_invert (tile2color, NULL);

  map_free (tile2color, true);

  return exec;
}

int exec_num_colors (executor_t* exec)
{
  ASSERT(exec != NULL, "Invalid NULL pointer to executor");

  return exec->color2tile->inSet->size;
}

int exec_tiles_per_color (executor_t* exec, int color)
{
  ASSERT ((color >= 0) && (color < exec_num_colors(exec)), "Invalid color provided");

  int* offsets = exec->color2tile->offsets;
  return offsets[color + 1] - offsets[color];
}

tile_t* exec_tile_at (executor_t* exec, int color, int ithTile, tile_region region)
{
  int tileID = exec->color2tile->values[exec->color2tile->offsets[color] + ithTile];
  tile_t* tile = exec->tiles->at (tileID);
  return (tile->region == region) ? tile : NULL;
}

void exec_free (executor_t* exec)
{
  tile_list* tiles = exec->tiles;
  tile_list::const_iterator it, end;
  for (it = tiles->begin(), end = tiles->end(); it != end; it++) {
    tile_free (*it);
  }
  delete tiles;
  map_free (exec->color2tile, true);
  delete exec;
}
