/*
 *  coloring.cpp
 *
 */

#include <string.h>

#include "coloring.h"

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
  return imap (set_cpy(iter2tile->inSet), set("colors", nTiles), iter2color, offsets);
}

map_t* color_kdistance (loop_list* loops, int seed, map_t* iter2tile, tile_list* tiles)
{
  return NULL;
}
