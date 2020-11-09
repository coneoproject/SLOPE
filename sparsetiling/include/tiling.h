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
#include "utils.h"
#include "schedule.h"
#include "common.h"

#ifdef __cplusplus
  extern"C" {
#endif


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
 * @param ignoreWAR
 *   if true, avoid tracking write-after-read dependencies, which decreases
 *   inspection time and, potentially, improves load balancing. This may be useful
 *   in unstructured mesh codes.
 */
void project_forward (loop_t* tiledLoop,
                      schedule_t* tilingInfo,
                      projection_t* prevLoopProj,
                      projection_t* seedLoopProj,
                      tracker_t* conflictsTracker,
                      bool ignoreWAR);

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
 * @param ignoreWAR
 *   if true, avoid tracking write-after-read dependencies, which decreases
 *   inspection time and, potentially, improves load balancing. This may be useful
 *   in unstructured mesh codes.
 */
void project_backward (loop_t* tiledLoop,
                       schedule_t* tilingInfo,
                       projection_t* prevLoopProj,
                       tracker_t* conflictsTracker,
                       bool ignoreWAR);

/*
 * Tile a parloop moving forward along the loop chain.
 *
 * @param curLoop
 *   the loop whose iterations will be colored and assigned a tile
 * @param prevLoopProj
 *   the projection of tiling up to curLoop
 */
schedule_t* tile_forward (loop_t* curLoop,
                         projection_t* prevLoopProj);

/*
 * Tile a parloop moving backward along the loop chain.
 *
 * @param curLoop
 *   the loop whose iterations will be colored and assigned a tile
 * @param prevLoopProj
 *   the projection of tiling up to curLoop
 */
schedule_t* tile_backward (loop_t* curLoop,
                          projection_t* prevLoopProj);

/*
 * Distribute the iterations of a tiled loop to tiles
 *
 * @param loop
 *   the tiled loop whose iterations will be assigned to tiles
 * @param loops
 *   the loop chain
 * @param tiles
 *   the list of tiles to be populated
 * @param iter2tile
 *   an integer map from iterations to tiles
 * @param direction
 *  the tiling direction
 */
void assign_loop (loop_t* loop,
                  loop_list* loops,
                  tile_list* tiles,
                  int* iter2tile,
                  direction_t direction);

/**************************************************************************/
#ifdef __cplusplus
  }
#endif
#endif
