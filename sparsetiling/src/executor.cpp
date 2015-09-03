/*
 *  executor.cpp
 *
 */

#include "executor.h"
#include "utils.h"

static void compute_local_ind_maps(loop_list* loops, tile_list* tiles);

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
  map_t* tile2color = map ("t2c", tile_set, color_set, tile2colorIndMap, nTiles);

  exec->tiles = tiles;
  exec->color2tile = map_invert (tile2color, NULL);

  // compute local indirection maps, for all tiles
  compute_local_ind_maps (insp->loops, tiles);

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

static void compute_local_ind_maps(loop_list* loops, tile_list* tiles)
{
  // aliases
  int nLoops = loops->size();
  int nTiles = tiles->size();

  /* For each loop spanned by a tile, take the global maps used in that loop and,
   * for each of them:
   * - access it by an iteration index
   * - store the accessed value in a tile's local map
   * This way, local iteration index 0 in the local map corresponds to the global
   * iteration index that a tile would have accessed first in a given loop; and so on.
   * This allows scanning indirection maps linearly, which should improve hardware
   * prefetching, instead of accessing a list of non-contiguous indices in a
   * global mapping.
   */
  loop_list::const_iterator lIt, lEnd;
  int i = 0;
  for (lIt = loops->begin(), lEnd = loops->end(); lIt != lEnd; lIt++, i++) {
    desc_list* descriptors = (*lIt)->descriptors;

    tile_list::const_iterator tIt, tEnd;
    for (tIt = tiles->begin(), tEnd = tiles->end(); tIt != tEnd; tIt++) {
      mapname_iterations* localMaps = new mapname_iterations;
      desc_list::const_iterator dIt, dEnd;
      for (dIt = descriptors->begin(), dEnd = descriptors->end(); dIt != dEnd; dIt++) {
        map_t* globalMap = (*dIt)->map;

        if (globalMap == DIRECT) {
          continue;
        }
        if (localMaps->find(globalMap->name) != localMaps->end()) {
          // avoid computing same local map more than once
          continue;
        }

        int* globalIndMap = globalMap->values;
        int arity = globalMap->size / globalMap->inSet->size;
        int tileLoopSize = (*tIt)->iterations[i]->size();

        iterations_list* localMap = new iterations_list (tileLoopSize*arity);
        localMaps->insert (mi_pair(globalMap->name, localMap));

        for (int e = 0; e < tileLoopSize; e++) {
          int element = (*tIt)->iterations[i]->at(e);
          for (int j = 0; j < arity; j++) {
            localMap->at(e*arity + j) = globalIndMap[element*arity + j];
          }
        }
      }
      (*tIt)->localMaps[i] = localMaps;
    }
  }
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
