/*
 *  coloring.cpp
 *
 */

#include <string.h>

#include "coloring.h"
#include "utils.h"

static int* color_apply (tile_list* tiles, map_t* iter2tile, int* colors)
{
  // aliases
  int itSetSize = iter2tile->inSet->size;
  int nTiles = iter2tile->outSet->size;
  int* offsets = iter2tile->offsets;

  int* iter2color = new int[itSetSize];

  for (int i = 0; i < nTiles; i++ ) {
    for (int j = offsets[i]; j < offsets[i + 1]; j++) {
      iter2color[j] = colors[i];
    }
    tiles->at(i)->color = colors[i];
  }

  return iter2color;
}

map_t* color_sequential (map_t* iter2tile, tile_list* tiles)
{
  // aliases
  int nTiles = iter2tile->outSet->size;

  int* offsets = new int[nTiles + 1];
  memcpy (offsets, iter2tile->offsets, sizeof(int)*(nTiles + 1));

  // each tile is assigned a different color. This way, the tiling algorithm,
  // which is based on colors, works seamless regardless of whether the
  // execution strategy is seqeuntial or parallel (shared memory)
  int* colors = new int[nTiles];
  for (int i = 0; i < nTiles; i++) {
    colors[i] = i;
  }

  int* iter2color = color_apply(tiles, iter2tile, colors);

  delete[] colors;

  // note we have as many colors as the number of tiles
  return imap ("i2c", set_cpy(iter2tile->inSet), set("colors", nTiles),
               iter2color, offsets);
}

map_t* color_shm (loop_t* loop, map_t* iter2tile, tile_list* tiles)
{
  // aliases
  map_t* seedMap = loop->seedMap;
  int nTiles = tiles->size();
  int seedSetSize = seedMap->inSet->size;
  int seedMapSize = seedMap->mapSize;
  int* seedIndMap = seedMap->indMap;

  ASSERT (seedMap, "Couldn't find a valid map for coloring a seed iteration space");

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

  // create the iteration to colors map
  int* iter2color = color_apply(tiles, iter2tile, colors);
  int* offsets = new int[nTiles + 1];
  memcpy (offsets, iter2tile->offsets, sizeof(int)*(nTiles + 1));

  delete[] work;
  delete[] colors;
  map_free (tile2iter);

  return imap ("i2c", set_cpy(iter2tile->inSet), set("colors", nColors),
               iter2color, offsets);
}
