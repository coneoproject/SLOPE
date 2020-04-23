/*
 *  coloring.cpp
 *
 */

#include <set>
#include <algorithm>

#include "coloring.h"
#include "utils.h"

static int* color_apply (tile_list* tiles, map_t* tile2iter, int* colors)
{
  // aliases
  int itSetSize = tile2iter->outSet->size;
  int nTiles = tile2iter->inSet->size;

  int* iter2color = new int[itSetSize];

  for (int i = 0; i < nTiles; i++ ) {
    // determine the tile iteration space
    int prevOffset = tile2iter->offsets[i];
    int nextOffset = tile2iter->offsets[i + 1];

    for (int j = prevOffset; j < nextOffset; j++) {
      iter2color[tile2iter->values[j]] = colors[i];
    }
    tiles->at(i)->color = colors[i];
  }

  return iter2color;
}

void color_sequential (inspector_t* insp)
{
  // aliases
  tile_list* tiles = insp->tiles;
  map_t* iter2tile = insp->iter2tile;
  int nTiles = iter2tile->outSet->size;

  // tiles are assigned colors in increasing order.
  // note: halo tiles always get the maximum colors
  int* colors = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    colors[i] = i;
  }

  map_t* tile2iter = map_invert (iter2tile, NULL);
  int* iter2color = color_apply(tiles, tile2iter, colors);

  map_free (tile2iter, true);
  delete[] colors;

  // note we have as many colors as the number of tiles
  insp->iter2color = map ("i2c", set_cpy(iter2tile->inSet), set("colors", nTiles),
                          iter2color, iter2tile->inSet->size*1);
}

void color_rand (inspector_t* insp)
{
  // aliases
  tile_list* tiles = insp->tiles;
  map_t* iter2tile = insp->iter2tile;
  int nCore = insp->tileRegions->core;
  int nTiles = iter2tile->outSet->size;

  // each tile is assigned a different color, in random order
  // note: halo tiles always get the maximum colors
  int* colors = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    colors[i] = i;
  }
  std::random_shuffle (colors, colors + nCore);

  map_t* tile2iter = map_invert (iter2tile, NULL);
  int* iter2color = color_apply(tiles, tile2iter, colors);

  map_free (tile2iter, true);
  delete[] colors;

  // note we have as many colors as the number of tiles
  insp->iter2color = map ("i2c", set_cpy(iter2tile->inSet), set("colors", nTiles),
                          iter2color, iter2tile->inSet->size*1);
}

void color_fully_parallel (inspector_t* insp)
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

  insp->iter2color = map ("i2c", set_cpy(iter2tile->inSet), set("colors", nTiles),
                          iter2color, iter2tile->inSet->size*1);
}

void color_diff_adj (inspector_t* insp, map_t* seedMap,
                     tracker_t* conflictsTracker, bool onlyCore)
{
  // aliases
  tile_list* tiles = insp->tiles;
  map_t* iter2tile = insp->iter2tile;
  int nTiles = tiles->size();
  int seedSetSize = seedMap->inSet->size;
  int outSetSize = seedMap->outSet->size;
  int* seedIndMap = seedMap->values;

  map_t* tile2iter = map_invert (iter2tile, NULL);

  // init colors
  int* colors = new int[nTiles];
  std::fill_n (colors, nTiles, -1);

  bool repeat = true;
  int nColor = 0;
  int nColors = 0;

  // array to work out the colors
  int* work = new int[outSetSize];

  // coloring algorithm
  while (repeat)
  {
    repeat = false;

    // zero out color array
    std::fill_n (work, outSetSize, 0);

    // start coloring tiles
    for (int i = 0; i < nTiles; i++)
    {
      // determine the tile iteration space
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
          int offset, size, element = tile2iter->offsets[*it];
          map_ofs(seedMap, element, &offset, &size);
          mask |= work[seedIndMap[offset + 0]];

          // When the frontier of a tile expands (at most k vertices), the given conflicts
          // cannot be captured by considering the adjacent tiles in the seed loop's iteration space.
          // Hence the colors of the conflicting tiles are added to the mask, in case they are not
          // adjacent in the seed loop's iteration space.
          if(colors[*it] != -1){
            mask |= 1 << colors[*it];
          }
        }

        for (int e = prevOffset; e < nextOffset; e++) {
          int offset, size, element = tile2iter->values[e];
          map_ofs(seedMap, element, &offset, &size);
          for (int j = 0; j < size; j++) {
            // set bits of mask
            mask |= work[seedIndMap[offset + j]];
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
            int offset, size, element = tile2iter->values[e];
            map_ofs(seedMap, element, &offset, &size);
            for (int j = 0; j < size; j++) {
              work[seedIndMap[offset + j]] |= mask;
            }
          }
        }
      }
    }
    // increment base level
    nColor += 32;
  }

  // shift up the halo tile colors, since these tiles must be executed after all core tiles
  int maxExecHaloColor = nColors;
  set_t* tileRegions = insp->tileRegions;
  if (onlyCore) {
    for (int i = 0; i < tileRegions->execHalo; i++) {
      colors[tileRegions->core + i] = maxExecHaloColor++;
    }
    maxExecHaloColor --;
  }
  else {
    for (int i = 0; i < tileRegions->execHalo; i++) {
      int newHaloColor = colors[tileRegions->core + i] + nColors;
      maxExecHaloColor = MAX(maxExecHaloColor, newHaloColor);
      colors[tileRegions->core + i] = newHaloColor;
    }
  }
  if (tileRegions->nonExecHalo > 0) {
    maxExecHaloColor += 1;
    colors[tileRegions->core + tileRegions->execHalo] = maxExecHaloColor;
  }
  nColors = maxExecHaloColor + 1;

  // create the iteration to colors map
  int* iter2color = color_apply(tiles, tile2iter, colors);

  delete[] work;
  delete[] colors;
  map_free (tile2iter, true);

  insp->iter2color = map ("i2c", set_cpy(iter2tile->inSet), set("colors", nColors),
                          iter2color, seedSetSize*1);
}
