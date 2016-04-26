/*
 *  tiling.h
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#ifndef _TILING_H_
#define _TILING_H_

#include <set>
#include <unordered_map>
#include <string>

#include "parloop.h"
#include "tile.h"

/**************************************************************************/

/*
 * Simple structure to map iteration set elements to a tile and a color
 */
typedef struct {
  /* set name identifier */
  std::string name;
  /* iteration set */
  int itSetSize;
  /* tiling of the iteration set */
  int* iter2tile;
  /* coloring of the iteration set */
  int* iter2color;
} iter2tc_t;

/*
 * Map iterations to tile IDs and colors.
 *
 * Note: the caller loses ownership of iter2tile and iter2color after calling
 * this function.
 */
iter2tc_t* iter2tc_init (std::string name,
                         int itSetSize,
                         int* iter2tile,
                         int* iter2color);

/*
 * Clone an iter2tc_t
 */
iter2tc_t* iter2tc_cpy (iter2tc_t* iter2tc);

/*
 * Destroy an iter2tc_t
 */
void iter2tc_free (iter2tc_t* iter2tc);


inline bool iter2tc_cmp (const iter2tc_t* a,
                         const iter2tc_t* b)
{
  return a->name < b->name;
}



/**************************************************************************/

/*
 * Represent a projection
 */
typedef std::set<iter2tc_t*, bool(*)(const iter2tc_t* a, const iter2tc_t* b)> projection_t;

/*
 * Initialize a new projection
 */
projection_t* projection_init();

/*
 * Destroy a projection
 */
void projection_free (projection_t* projection);



/**************************************************************************/

/* Prototypes and data structures for the tiling and projection functions
 *
 * Notes:
 * 1) Since tiling must be fast, we prefer to have separated functions for forward
 * and backward tiling (or projection), rather than a single function using an
 * if-then-else construct or function pointers to apply the MAX/MIN operators.
 *
 * 2) Tiling and projection make use of "trackers". A tracker is used to map
 * conflicting tiles; that is, tiles having the same color that, due to the tiling
 * process, became adjacent. In a tracker, an entry ``1 -> {2, 3}'' means that
 * tile 1 is in conflict with tiles 2 and 3.
 */

/* Tracker: a map from tile IDs to a set of tile IDs */
typedef std::set<int> index_set;
typedef std::map<int, index_set> tracker_t;

/*
 * Project tiling and coloring of an iteration set to all sets that are
 * touched (read, incremented, written) by a parloop /i/, as tiling goes forward.
 * This produces the required information for calling tile_forward on parloop /i+1/.
 * In particular, update prevLoopProj by exploiting information in tilingInfo. Also,
 * seedLoopProj is updated if new sets are encountered.
 *
 * @param tiledLoop
 *   the tiled loop, which contains the descriptors required to perform the
 *   projection.
 * @param tilingInfo
 *   the tiling and coloring of tiledLoop.
 * @param prevLoopProj
 *   a set of iteration-sets to tile-color mappings, which represents the
 *   projection of tiling at loop_{i-1}.
 * @param seedLoopProj
 *   the set of iteration-sets to tile-color mappings "closest" to the seed loop.
 *   Here closest means that these mappings represent the projection of the sets
 *   accessed as tiling forward that has to be used for backward tiling.
 * @param conflictsTracker
 *   track conflicting tiles encountered by each tile during the tiling process
 */
void project_forward (loop_t* tiledLoop,
                      iter2tc_t* tilingInfo,
                      projection_t* prevLoopProj,
                      projection_t* seedLoopProj,
                      tracker_t* conflictsTracker);

/*
 * Project tiling and coloring of an iteration set to all sets that are
 * touched (read, incremented, written) by a parloop /i/, as tiling goes backward.
 * This produces the required information for calling tile_backward on a parloop /i-1/.
 * In particular, update prevLoopProj by exploiting information in tilingInfo.
 *
 * @param tiledLoop
 *   the tiled loop, which contains the descriptors required to perform the
 *   projection.
 * @param tilingInfo
 *   the tiling and coloring of tiledLoop.
 * @param prevLoopProj
 *   a set of iteration-sets to tile-color mappings, which represents the
 *   projection of tiling at loop_{i+1}.
 * @param conflictsTracker
 *   track conflicting tiles encountered by each tile during the tiling process
 */
void project_backward (loop_t* tiledLoop,
                       iter2tc_t* tilingInfo,
                       projection_t* prevLoopProj,
                       tracker_t* conflictsTracker);

/*
 * Tile a parloop moving forward along the loop chain.
 *
 * @param curLoop
 *   the loop whose iterations will be colored and assigned a tile
 * @param prevLoopProj
 *   the projection of tiling up to curLoop
 */
iter2tc_t* tile_forward (loop_t* curLoop,
                         projection_t* prevLoopProj);

/*
 * Tile a parloop moving backward along the loop chain.
 *
 * @param curLoop
 *   the loop whose iterations will be colored and assigned a tile
 * @param prevLoopProj
 *   the projection of tiling up to curLoop
 */
iter2tc_t* tile_backward (loop_t* curLoop,
                          projection_t* prevLoopProj);

/**************************************************************************/

#endif
