/*
 *  loop.h
 *
 * Data structures and prototypes for parallel loops
 */

#ifndef _PARLOOP_H_
#define _PARLOOP_H_

#include <vector>
#include <string>

#include "descriptor.h"

/*
 * Represent a parloop
 */
typedef struct {
  /* name/identifier of the parloop */
  std::string name;
  /* size of the iteration set */
  set_t* set;
  /* list of maps used to execute the kernel in this loop */
  desc_list* descriptors;
  /* track coloring of the parloop */
  int* coloring;
  /* track tiling of the parloop */
  int* tiling;

} loop_t;

typedef std::vector<loop_t*> loop_list;

#endif
