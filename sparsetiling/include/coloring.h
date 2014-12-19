/*
 *  coloring.h
 *
 */

#ifndef _COLORING_H_
#define _COLORING_H_

#include "parloop.h"

/*
 * Assign increasing colors to the various tiles.
 *
 * @param iter2tile
 *   map from iteration set to tiles
 * @return
 *   a map from iteration set to colors
 */
map_t* color_sequential (map_t* iter2tile);

/*
 * Assign colors to the tiles such that two tiles within a distance of k steps
 * cannot be assigned the same color.
 *
 * The value of k comes from an analysis of the loop chain, specifically from the
 * way parloops write/increment and read datasets.
 *
 * @param loops
 *   the loop chain
 * @param
 *   start point of tiling in the loop chain
 * @param iter2tile
 *   map from iteration set to tiles
 * @return
 *   a map from iteration set to colors, where tiles' colors are enforced to be
 *   different if the tiles are within k steps
 */
map_t* color_kdistance (loop_list* loops, int seed, map_t* iter2tile);

#endif
