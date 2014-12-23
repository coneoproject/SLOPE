/*
 *  executor.h
 *
 * Executor data structures and prototypes
 */

#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#include "inspector.h"

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
 * Destroy an executor
 *
 * @param exec
 *   the executor data structure
 */
void exec_free (executor_t* exec);

#endif
