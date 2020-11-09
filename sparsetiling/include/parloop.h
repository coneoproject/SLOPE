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


#ifdef __cplusplus
  extern"C" {
#endif

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
  /* number of halos in the mappings */
  int nhalos;

} loop_t;

typedef std::vector<loop_t*> loop_list;

/*
 * Search for a map from the loop iteration set to another set (not a subset),
 * or, alternatively, from another set (not a subset) to the loop iteration set.
 * The map is stored in the /seedMap/ field of the /loop_t/ structure. The map
 * relates the loop iteration set to another set: this makes it suitable for
 * shared memory coloring of /loop/.
 *
 * @param loop
 *   the loop for which a map needs be retrieved
 * @param loops
 *   (optional) if a suitable map is not found among the access descriptors of
 *   /loop/, search for it in the other loops the inspector is aware of
 * @return
 *   true if one such map is found. false otherwise
 */
bool loop_load_seed_map (loop_t* loop,
                         loop_list* loops = NULL);

/*
 * @return
 *   /true/ if the loop is direct (i.e., all access descriptors represent direct
 *   data accesses), /false/ otherwise
 */
bool loop_is_direct (loop_t* loop);

/*
 * @return
 *   /true/ if the two loops have same iteration space, /false/ otherwise
 */
inline bool loop_eq_itspace (loop_t* loop_a,
                             loop_t* loop_b)
{
  return loop_a && loop_b && set_eq (loop_a->set, loop_b->set);
}

#ifdef __cplusplus
  }
#endif
#endif
