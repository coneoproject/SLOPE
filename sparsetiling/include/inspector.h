/*
 *  inspector.h
 *
 * Define data structure and prototypes to build an inspector
 */

#ifndef _INSPECTOR_H_
#define _INSPECTOR_H_

#include "parloop.h"

/*
 * The inspector main data structure.
 */
typedef struct {
  /* the base loop index */
  int baseLoop;
  /* initial tile size */
  int tileSize;
  /* list of loops spanned by a tile */
  loop_list* loops;

} inspector_t;

enum insp_info {INSP_OK, INSP_ERR};

/*
 * Initialize a new inspector
 *
 * input:
 * loops: list of loops crossed by the inspector
 * baseLoop: index of the base loop
 * tileSize: average tile size after partitioning of the base loop's iteration set
 *
 * output:
 * an inspector data structure
 */
inspector_t* insp_init (loop_list* loops, int baseLoop, int tileSize);

/*
 * Inspect a sequence of parloops and compute a tiled scheduling
 *
 * input:
 * insp: the main inspector data structure, already defined over a range of parloops
 * seed: start point of the tiling (a number between 0 and the number of
 *       parloops spanned by the inspector)
 *
 * output:
 * on return from the function, insp will contain a list of tiles, each tile
 * characterized by a list of iterations that are supposed to be executed, for each
 * crossed parloop
 */
insp_info insp_run (inspector_t* insp, int seed);

/*
 * Destroy an inspector
 */
void insp_free (inspector_t* insp);

#endif
