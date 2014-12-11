/*
 *  inspector.cpp
 *
 * Implement inspector routines
 */

#include <stdlib.h>

#include "inspector.h"
#include "utils.h"
#include "partitioner.h"

inspector_t* insp_init (loop_list* loops, int baseLoop, int tileSize)
{
  ASSERT((baseLoop >= 0) && (baseLoop < loops->size()), "Invalid base loop index");

  inspector_t* insp = (inspector_t*) malloc(sizeof(inspector_t));
  insp->loops = loops;
  insp->baseLoop = baseLoop;
  insp->tileSize = tileSize;
  return insp;
}

insp_info insp_run (inspector_t* insp, int seed)
{
  // aliases
  loop_list* loops = insp->loops;
  int tileSize = insp->tileSize;

  ASSERT((seed >= 0) && (seed < loops->size()), "Invalid tiling start point");
  loop_t* baseLoop = (*loops)[seed];

  // partition the iteration set of the base loop
  partition (baseLoop, tileSize);

  return INSP_OK;
}

void insp_free (inspector_t* insp)
{
  free (insp);
}
