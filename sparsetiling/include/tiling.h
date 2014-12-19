/*
 *  tiling.h
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#ifndef _TILING_H_
#define _TILING_H_

#include <set>

#include <string.h>

#include "parloop.h"

/*
 * Bind each element of an iteration set to a tile and a color
 */
typedef struct {
  /* set name identifier */
  char* setName;
  /* iteration set */
  int itSetSize;
  /* tiling of the iteration set */
  int* iter2tile;
  /* coloring of the iteration set */
  int* iter2color;
} iter2tc_t;

/*
 * A projection_t can be implemented by any container provided that the semantics
 * of the container is such that its elements are unique.
 */
inline bool iter2tc_cmp(const iter2tc_t* a, const iter2tc_t* b)
{
  return strcmp(a->setName, b->setName) < 0;
}
typedef std::set<iter2tc_t*, bool(*)(const iter2tc_t* a, const iter2tc_t* b)> projection_t;

/*
 * Bind iterations to tile IDs and colors.
 *
 * Note: the caller loses ownership of iter2tile and iter2color after calling
 * this function. Access to these two maps becomes therefore undefined.
 */
iter2tc_t* iter2tc_init (char* setName, int itSetSize, int* iter2tile, int* iter2color);

/*
 * Clone an iter2tc_t
 */
iter2tc_t* iter2tc_cpy (iter2tc_t* iter2tc);

/*
 * Destroy an iter2tc_t
 */
void iter2tc_free (iter2tc_t* iter2tc);

/*
 * Free resources associated with a projection_t
 */
void projection_free (projection_t* projection);

// Note: the following functions are classified as either for forward or backward
// tiling. Since the tiling operations need to be fast, we prefer to keep them
// separated instead of using a single "project" and a single "tile" functions plus
// function pointers to MAX/MIN (that is, we stick to inline calls).

/*
 * Project tiling and coloring of an iteration set to all sets that are
 * touched (read, incremented, written) by the parloop, as tiling goes forward.
 * This produces the required information for calling tile_forward.
 *
 * @param tiledLoop
 *   the tiled loop, which contains the descriptors required to perform the
 *   projection.
 * @param tilingInfo
 *   the tiling and coloring of tiledLoop.
 * @param prevLoopProj
 *   a set of iteration-sets to tile-color mappings, which represents the
 *   projection of tiling at loop_{i-1}.
 * @param baseLoopProj
 *   the set of iteration-sets to tile-color mappings "closest" to the seed loop.
 *   Here closest means that these mappings represent the projection of the sets
 *   accessed as tiling forward that has to be used for backward tiling.
 * @return
 *   update prevLoopProj by exploiting information in tilingInfo. Also, baseLoopProj
 *   is updated if new sets are encountered.
 */
void project_forward (loop_t* tiledLoop, iter2tc_t* tilingInfo,
                      projection_t* prevLoopProj, projection_t* baseLoopProj);

/*
 * Project tiling and coloring of an iteration set to all sets that are
 * touched (read, incremented, written) by the parloop, as tiling goes backward.
 * This produces the required information for calling tile_backward.
 *
 * @param tiledLoop
 *   the tiled loop, which contains the descriptors required to perform the
 *   projection.
 * @param tilingInfo
 *   the tiling and coloring of tiledLoop.
 * @param prevLoopProj
 *   a set of iteration-sets to tile-color mappings, which represents the
 *   projection of tiling at loop_{i+1}.
 * @return
 *   Update prevLoopProj by exploiting information in tilingInfo.
 */
void project_backward (loop_t* tiledLoop, iter2tc_t* tilingInfo,
                       projection_t* prevLoopProj);

/*
 * Tile a parloop when going forward along the loop chain.
 */
iter2tc_t* tile_forward (loop_t* curLoop, projection_t* prevLoopProj);

/*
 * Tile a parloop when going backward along the loop chain.
 */
void tile_backward ();

#endif
