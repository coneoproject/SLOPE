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
  /* loop chain position */
  int index;
  /* size of the iteration set */
  set_t* set;
  /* list of maps used to execute the kernel in this loop */
  desc_list* descriptors;
  /* track coloring of the parloop */
  int* coloring;
  /* track tiling of the parloop */
  int* tiling;
  /* map used for seed coloring (NULL if the loop is not the tiling seed) */
  map_t* seedMap;

} loop_t;

typedef std::vector<loop_t*> loop_list;

/*
 * Retrieve a map from the loop's iteration set to another set (not a subset).
 *
 * @return
 *   true if one such map is found and successfully stored in the corresponding
 *   loop_t's field (
 */
bool loop_load_full_map (loop_t* loop);

#endif
