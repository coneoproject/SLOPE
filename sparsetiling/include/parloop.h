/*
 *  loop.h
 *
 * Data structures and prototypes for parallel loops
 */

#ifndef _PARLOOP_H_
#define _PARLOOP_H_

#include "descriptors.h"

/*
 * Represent a parloop
 */
typedef struct {
  /* name/identifier of the parloop */
  char* loopName;
  /* size of the iteration set */
  int setSize;
  /* list of maps used to execute the kernel in this loop */
  desc_list* descriptors;

} loop_t;

typedef std::list<loop_t*> loop_list;

#endif
