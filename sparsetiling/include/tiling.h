/*
 *  tiling.h
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#ifndef _TILING_H_
#define _TILING_H_

#include <vector>

#include "descriptors.h"
#include "parloop.h"

/*
 * Bind each element of an iteration set to a tile and a color
 */
typedef struct {
  /* iteration set */
  int itsetSize;
  /* tiling of the iteration set */
  int* iter2tile;
  /* number of tiles */
  int nTiles;
  /* coloring of the iteration set */
  int* iter2color;
  /* number of colors */
  int nColors;
  /* offsets in the two maps */
  int* offsets;
} iter2tc_t;

typedef std::vector<iter2tc_t*> projection_list;

/*
 * Bind iterations to tile IDs and colors.
 *
 * Note: the caller loses ownership of iter2tile and iter2color after calling
 * this function. Access to these two maps becomes therefore undefined.
 */
iter2tc_t* make_iter2tc (map_t* iter2tile, map_t* iter2color);

/*
 * Destroy an iter2tc_t
 */
void iter2tc_free (iter2tc_t* iter2tc);

// Note: the following functions are classified as either for forward or backward
// tiling. Since the tiling operations need to be fast, we prefer to keep them
// separated instead of using a single "project" and a single "tile" functions plus
// function pointers to MAX/MIN (that is, we stick to inline calls).

/*
 * Project tiling and coloring of an iteration set to the other sets that are
 * touched by the parloop, as tiling is going forward. This produces information
 * for the subsequent call to tile_forward.
 *
 * @param tiledLoop
 *   the tiled loop, which contains the descriptors required to perform the
 *   projection
 * @param tilingInfo
 *   brings information about the result of tiling, including the tile each
 *   iteration set element belongs to as well as the color
 * @return
 *   a list of iteration-sets to tile-color mappings, which represents the
 *   projection of tiling loop_{i} necessary for tiling loop_{i+1}
 */
projection_list* project_forward (loop_t* tiledLoop, iter2tc_t tilingInfo);

/*
 * Project tiling and coloring of an iteration set to the other sets that are
 * touched by the parloop, as tiling is going backward. This produces information
 * for the subsequent call to tile_backward.
 *
 * @param tiledLoop
 *   the tiled loop, which contains the descriptors required to perform the
 *   projection
 * @param tilingInfo
 *   brings information about the result of tiling, including the tile each
 *   iteration set element belongs to as well as the color
 * @return
 *   a list of iteration-sets to tile-color mappings, which represents the
 *   projection of tiling loop_{i} necessary for tiling loop_{i-1}
 */
projection_list* project_backward (loop_t* tiledLoop, iter2tc_t tilingInfo);

/*
 * Tile a parloop when going forward along the loop chain.
 */
void tile_forward ();

/*
 * Tile a parloop when going backward along the loop chain.
 */
void tile_backward ();

#endif
