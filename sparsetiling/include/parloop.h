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
 * Retrieve a map from the loop iteration set to another set (not a subset), and
 * store it in the /seedMap/ field of the /loop_t/ structure.
 *
 * @return
 *   true if one such map is found. false otherwise
 */
bool loop_load_full_map (loop_t* loop);

/*
 * @return
 *   true if the loop is direct (i.e., all access descriptors represent direct
 *   data accesses), false otherwise
 */
bool loop_is_direct (loop_t* loop);

#endif
