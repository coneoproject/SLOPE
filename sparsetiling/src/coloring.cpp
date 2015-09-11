/*
 *  coloring.cpp
 *
 */

#include <string.h>

#include "coloring.h"
#include "utils.h"

static int* color_apply (tile_list* tiles, map_t* tile2iter, int* colors)
{
  // aliases
  int itSetSize = tile2iter->outSet->size;
  int nTiles = tile2iter->inSet->size;

  int* iter2color = new int[itSetSize];

  for (int i = 0; i < nTiles; i++ ) {
    // determine the tile's iteration space
    int prevOffset = tile2iter->offsets[i];
    int nextOffset = tile2iter->offsets[i + 1];

    for (int j = prevOffset; j < nextOffset; j++) {
      iter2color[j] = colors[i];
    }
    tiles->at(i)->color = colors[i];
  }

  return iter2color;
}

map_t* color_sequential (inspector_t* insp)
{
  // aliases
  tile_list* tiles = insp->tiles;
  map_t* iter2tile = insp->iter2tile;
  int nTiles = iter2tile->outSet->size;

  // each tile is assigned a different color. This way, the tiling algorithm,
  // which is based on colors, works seamless regardless of whether the
  // execution strategy is sequential or parallel (shared memory).
  // Note: halo tiles always get the maximum colors
  int* colors = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    colors[i] = i;
  }

  map_t* tile2iter = map_invert (iter2tile, NULL);
  int* iter2color = color_apply(tiles, tile2iter, colors);

  map_free (tile2iter, true);
  delete[] colors;

  // note we have as many colors as the number of tiles
  return map ("i2c", set_cpy(iter2tile->inSet), set("colors", nTiles), iter2color,
              iter2tile->inSet->size*1);
}

map_t* color_fully_parallel (inspector_t* insp)
{
  // aliases
  tile_list* tiles = insp->tiles;
  map_t* iter2tile = insp->iter2tile;
  int nTiles = iter2tile->outSet->size;

  // A same color is assigned to all tiles. This is because it was found that
  // all tiles can safely run in parallel.
  // Note: halo tiles are an exception, since they always get a higher color
  int* colors = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    if (tiles->at(i)->region == LOCAL) {
      colors[i] = 0;
    }
    if (tiles->at(i)->region == EXEC_HALO) {
      colors[i] = 1;
    }
    if (tiles->at(i)->region == NON_EXEC_HALO) {
      colors[i] = 2;
    }
  }

  map_t* tile2iter = map_invert (iter2tile, NULL);
  int* iter2color = color_apply(tiles, tile2iter, colors);

  map_free (tile2iter, true);
  delete[] colors;

  // note we have as many colors as the number of tiles
  return map ("i2c", set_cpy(iter2tile->inSet), set("colors", nTiles), iter2color,
              iter2tile->inSet->size*1);
}

map_t* color_shm (inspector_t* insp, map_t* seedMap, tracker_t* conflictsTracker)
{
  // aliases
  tile_list* tiles = insp->tiles;
  map_t* iter2tile = insp->iter2tile;
  int nTiles = tiles->size();
  int seedSetSize = seedMap->inSet->size;
  int seedMapSize = seedMap->size;
  int* seedIndMap = seedMap->values;

  map_t* tile2iter = map_invert (iter2tile, NULL);
  int seedMapAriety = seedMapSize / seedSetSize;

  // init colors
  int* colors = new int[nTiles];
  std::fill_n (colors, nTiles, -1);

  bool repeat = true;
  int nColor = 0;
  int nColors = 0;

  // array to work out the colors
  int* work = new int[seedMapSize];

  // coloring algorithm
  while (repeat)
  {
    repeat = false;

    // zero out color array
    std::fill_n (work, seedMapSize, 0);

    // start coloring tiles
    for (int i = 0; i < nTiles; i++)
    {
      // determine the tile's iteration space
      int prevOffset = tile2iter->offsets[i];
      int nextOffset = tile2iter->offsets[i + 1];

      if (colors[i] == -1)
      {
        unsigned int mask = 0;

        // prevent tiles that are known to conflict if a "standard" coloring is
        // employed from being assigned the same color. For this, access the color,
        // in the work array, of the first iteration of each conflicting tile
        index_set tileConflicts = (*conflictsTracker)[i];
        index_set::const_iterator it, end;
        for (it = tileConflicts.begin(), end = tileConflicts.end(); it != end; it++) {
          mask |= work[seedIndMap[tile2iter->offsets[*it]*seedMapAriety + 0]];
        }

        for (int e = prevOffset; e < nextOffset; e++) {
          for (int j = 0; j < seedMapAriety; j++) {
            // set bits of mask
            mask |= work[seedIndMap[e*seedMapAriety + j]];
          }
        }

        // find first bit not set
        int color = ffs(~mask) - 1;
        if (color == -1) {
          // run out of colors on this pass
          repeat = true;
        }
        else
        {
          colors[i] = nColor + color;
          mask = 1 << color;
          nColors = MAX(nColors, nColor + color + 1);
          for (int e = prevOffset; e < nextOffset; e++) {
            for (int j = 0; j < seedMapAriety; j++) {
              work[seedIndMap[e*seedMapAriety + j]] |= mask;
            }
          }
        }
      }
    }
    // increment base level
    nColor += 32;
  }

  // assign maximum color to halo tiles, since these must be executed last
  set_t* tileRegions = insp->tileRegions;
  if (tileRegions->execHalo > 0) {
    colors[tileRegions->core] = nColors++;
  }
  if (tileRegions->nonExecHalo > 0) {
    colors[tileRegions->core + tileRegions->execHalo] = nColors++;
  }

  // create the iteration to colors map
  int* iter2color = color_apply(tiles, tile2iter, colors);

  delete[] work;
  delete[] colors;
  map_free (tile2iter, true);

  return map ("i2c", set_cpy(iter2tile->inSet), set("colors", nColors), iter2color,
              seedSetSize*1);
}
