/*
 *  executor.h
 *
 * Executor data structures and prototypes
 */

#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#include "inspector.h"
#include "utils.h"

/*
 * The executor main data structure.
 */
typedef struct {
  /* list of tiles */
  tile_list* tiles;
  /* map from colors to tiles */
  map_t* color2tile;

} executor_t;


/*
 * Initialize a new executor
 *
 * @param insp
 *   the inspector data structure
 */
executor_t* exec_init (inspector_t* insp);

/*
 * Return the total number of colors computed
 *
 * @param exec
 *   the executor data structure
 */
int exec_num_colors (executor_t* exec);

/*
 * Return the total number of tiles for a given color
 *
 * @param exec
 *   the executor data structure
 * @param color
 *   the color for which the number of tiles is retrieved
 */
int exec_tiles_per_color (executor_t* exec, int color);

/*
 * Return the ith tile having a given color
 *
 * @param exec
 *   the executor data structure
 * @param color
 *   the color for which the ith tile is retrieved
 * @param ithTile
 *   the tile that needs be retrieved having a given color
 */
tile_t* exec_tile_at (executor_t* exec, int color, int ithTile);

/*
 * Destroy an executor
 *
 * @param exec
 *   the executor data structure
 */
void exec_free (executor_t* exec);

#endif
