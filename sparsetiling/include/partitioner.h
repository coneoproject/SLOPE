/*
 *  partitioner.h
 *
 * Interface to routines for partitioning the iteration set
 */

#ifndef _PARTITIONER_H_
#define _PARTITIONER_H_

#include "inspector.h"

/*
 * Partition the /loop/ iteration set.
 *
 * @param insp
 *   the inspector data structure
 * @param loop
 *   the loop whose iteration set is partitioned
 * @param partSize
 *   average tile size for partitioning
 * @return
 *   a 2-tuple, in which the first entry is map from /loop/ iterations to tiles,
 *   while the second entry is the list of created tiles
 */
std::pair<map_t*, tile_list*> partition (inspector_t* insp, loop_t* loop, int tileSize);

#endif
