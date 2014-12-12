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
 * Input:
 * loop: the loop whose datasets are beign colored
 */
void color_sequential (loop_t* loop);

/*
 * Assign colors to the tiles such that two tiles within a distance of k steps
 * cannot be assigned the same color.
 *
 * The value of k comes from an analysis of the loop chain, specifically from the
 * way parloops write/increment and read datasets.
 *
 * Input:
 * loop: the loop whose datasets are beign colored
 */
void color_kdistance (loop_t* loop);

#endif
