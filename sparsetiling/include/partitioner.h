/*
 *  partitioner.h
 *
 * Interface to routines for partitioning the iteration set
 */

#ifndef _PARTITIONER_H_
#define _PARTITIONER_H_

#include "parloop.h"
#include "tile.h"

/*
 * Partition the /loop/ iteration set.
 *
 * @param loop
 *   the loop whose iteration set is partitioned
 * @param partSize
 *   average tile size for partitioning
 * @param crossedLoops
 *   number of loops crossed by a tile (i.e., length of the loop chain)
 * @return
 *   a 2-tuple, in which the first entry is map from /loop/ iterations to tiles,
 *   while the second entry is the list of created tiles
 */
std::pair<map_t*, tile_list*> partition (loop_t* loop, int tileSize, int crossedLoops);

#endif
