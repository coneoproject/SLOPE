/*
 *  coloring.h
 *
 */

#ifndef _COLORING_H_
#define _COLORING_H_

#include "inspector.h"
#include "tiling.h"

/*
 * Assign increasing colors to the tiles.
 *
 * @param insp
 *   the inspector data structure
 * @return
 *   build up the /iter2color/ field in /insp/
 */
void color_sequential (inspector_t* insp);

/*
 * Assign different colors to the tiles, in random order.
 *
 * @param insp
 *   the inspector data structure
 * @return
 *   build up the /iter2color/ field in /insp/
 */
void color_rand (inspector_t* insp);

/*
 * Assign the same color to all tiles. This means all tiles can run in parallel.
 * The only exceptions are the halo tiles, which get assigned a higher color.
 *
 * @param insp
 *   the inspector data structure
 * @return
 *   build up the /iter2color/ field in /insp/
 */
void color_fully_parallel (inspector_t* insp);

/*
 * Assign colors to tiles such that two adjacent tiles are not assigned the same color.
 *
 * @param insp
 *   the inspector data structure
 * @param seedMap
 *   a map used in the indirect seed loop to determine adjacent tiles
 * @param conflictsTracker
 *   track tiles that despite not being adjacent should not be assigned the same color
 * @return
 *   build up the /iter2color/ field in /insp/
 */
void color_shm (inspector_t* insp, map_t* seedMap, tracker_t* conflictsTracker);

#endif
