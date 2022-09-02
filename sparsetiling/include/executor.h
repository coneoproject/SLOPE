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
 *   the inspector from which the executor is built
 */
executor_t* exec_init (inspector_t* insp);

/*
 * Return the number of colors computed
 *
 * @param exec
 *   the executor data structure
 */
int exec_num_colors (executor_t* exec);

/*
 * Return the number of tiles for a given color
 *
 * @param exec
 *   the executor data structure
 * @param color
 *   the color for which the number of tiles is retrieved
 */
int exec_tiles_per_color (executor_t* exec,
                          int color);

/*
 * Return the i-th tile with given color
 *
 * @param exec
 *   the executor data structure
 * @param color
 *   the color of the tile
 * @param ithTile
 *   the ID of the tile that needs be retrieved
 * @param region
 *   a flag to specify the tile region; if this does not match with the retrieved
 *   tile's region, then NULL is returned
 */
tile_t* exec_tile_at (executor_t* exec,
                      int color,
                      int ithTile,
                      tile_region region = LOCAL);

/*
 * Destroy an executor
 */
void exec_free (executor_t* exec);

/*
 * Renumber mappings to satisfy OP2's data arrangements
 */
void create_mapped_iterations(inspector_t* insp, executor_t* exec);

#endif
