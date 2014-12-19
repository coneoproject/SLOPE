/*
 *  partitioner.h
 *
 * Interface to routines for partitioning the iteration set
 */

#ifndef _PARTITIONER_H_
#define _PARTITIONER_H_

#include "parloop.h"

enum part_info {PART_OK, PART_ERR};

/*
 * Partition the mesh based on a map from entities to vertices.
 *
 * input:
 * loop: the loop whose iteration set is partitioned
 * partSize: average size of the tiles the partitioning returns
 *
 * output:
 * a map from the loop's iteration set to tiles
 */
map_t* partition (loop_t* loop, int tileSize);

#endif
