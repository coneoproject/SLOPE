/*
 *  inspector.cpp
 *
 * Implement inspector routines
 */

#include <stdlib.h>

#include "inspector.h"
#include "utils.h"

inspector_t* insp_init (loop_list* loops, int baseLoop, int tileSize)
{
  ASSERT((baseLoop >= 0) && (baseLoop < loops->size()), "Invalid base loop index");

  inspector_t* insp = (inspector_t*) malloc(sizeof(inspector_t));
  insp->loops = loops;
  insp->baseLoop = baseLoop;
  insp->tileSize = tileSize;
  return insp;
}

void insp_free (inspector_t* insp)
{
  free (insp);
}
