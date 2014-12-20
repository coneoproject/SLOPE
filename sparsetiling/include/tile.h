/*
 *  tile.h
 *
 */

#ifndef _TILE_H_
#define _TILE_H_

#include <vector>

#include "descriptor.h"

typedef std::vector<int> iterations_list;

typedef struct {
  /* number of parloops spanned by the tile */
  int spannedLoops;
  /* list of iterations owned by the tile, for each parloop */
  iterations_list** iterations;
  /* color of the tile */
  int color;
} tile_t;

typedef std::vector<tile_t*> tile_list;

/*
 * Initialize a tile
 *
 * input:
 * spannedLoops: number of loops the tile spans
 */
tile_t* tile_init (int spannedLoops);

/*
 * Add iterations to one loop spanned by the tile
 *
 * input:
 * tiles: the tiles the iterations are added to
 * loopID: identifier of the loop spanned by the tile for which iterations are added
 * iter2tile: map from iterations to tile identifiers
 *
 * output:
 * the tiles will contain the iterations they have to execute for the given loop
 */
void tile_assign_loop (tile_list* tiles, int loopID, map_t* iter2tile);

/*
 * Free resources associated with the tile
 */
void tile_free (tile_t* tile);

#endif
