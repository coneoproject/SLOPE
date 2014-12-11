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
 * Destroy an inspector
 */
void insp_free (inspector_t* insp);

#endif
