/*
 *  coloring.h
 *
 */

#ifndef _COLORING_H_
#define _COLORING_H_

#include "inspector.h"

/*
 * Assign increasing colors to the various tiles.
 *
 * @param insp
 *   the inspector data structure
 * @return
 *   a map from iteration set to colors
 */
map_t* color_sequential (inspector_t* insp);

/*
 * Assign a same color to all tiles. This means all tiles will run in parallel.
 * The only exceptions are the halo tiles, which get assigned a higher color
 *
 * @param insp
 *   the inspector data structure
 * @return
 *   a map from iteration set to colors
 */
map_t* color_fully_parallel (inspector_t* insp);

/*
 * Assign colors to tiles such that two adjacent tiles are not assigned the same color.
 *
 * @param insp
 *   the inspector data structure
 * @param seedMap
 *   a map used in the indirect seed loop
 * @param conflictsTracker
 *   track conflicting tiles encountered during the tiling process
 * @return
 *   a map from iteration set elements to colors, where tile colors are enforced to be
 *   different if tiles are adjacent (e.g., there is an edge connecting them)
 */
map_t* color_shm (inspector_t* insp, map_t* seedMap, tracker_t* conflictsTracker);

#endif
