/*
 *  coloring.cpp
 *
 */

#include "coloring.h"

static int* color_apply (tile_list* tiles, map_t* iter2tile, int* colors)
{
  // aliases
  int itSetSize = iter2tile->inSet->size;
  int nTiles = iter2tile->outSet->size;
  int* offsets = iter2tile->offsets;

  int* iter2color = (int*) malloc (itSetSize*sizeof(int));

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
  int* offsets = iter2tile->offsets;

  // each tile is assigned a different color. This way, the tiling algorithm,
  // which is based on colors, works seamless regardless of whether the
  // execution strategy is seqeuntial or parallel (shared memory)
  int* colors = (int*) malloc (nTiles * sizeof(int));
  for (int i = 0; i < nTiles; i++) {
    colors[i] = i;
  }

  int* iter2color = color_apply(tiles, iter2tile, colors);

  // note we have as many colors as the number of tiles
  set_t* colorSet = set ("colors", nTiles);
  return imap (iter2tile->inSet, colorSet, iter2color, offsets);
}

map_t* color_kdistance (loop_list* loops, int seed, map_t* iter2tile, tile_list* tiles)
{
  return NULL;
}
