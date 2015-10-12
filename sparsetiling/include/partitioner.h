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
 * @return
 *   build up the /tiles/ and /iter2tile/ fields in /insp/
 */
void partition (inspector_t* insp);

#endif
