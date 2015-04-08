/*
 * set.h
 *
 */

#ifndef _SET_H_
#define _SET_H_

#include <set>

#include <string>

/*
 * Represent a set
 */
typedef struct {
  /* identifier name of the set */
  std::string name;
  /* size of the local set not touching halo regions */
  int size;
  /* size of the local set touching halo regions */
  int owned;
  /* size of the halo region (will be executed over redundantly) */
  int execHalo;
  /* size of the halo region that will only be read */
  int nonExecHalo;
  /* subset flag */
  bool isSubset;
} set_t;

typedef std::set<set_t*> set_list;

/*
 * Initialize a set
 *
 * If [owned, execHalo, nonExecHalo] are not specified, sequential execution
 * is assumed. Otherwise, the logical division into set elements is: ::
 *
 *     [0, size)
 *     [size, owned)
 *     [owned, execHalo)
 *     [execHalo, nonExecHalo).
 *
 * Where:
 *
 *    - `size`: all of the owned set elements, not touching the halo region; \
 *              this is the actual "size" of the "local" iteration space
 *    - `owned`: the subset of elements touching the halo region
 *    - `execHalo`: not owned, but executed over redundantly. The extent of \
 *                  this region should be proportial to the depth of tiling
 *    - `nonExecHalo`: read when executing the halo region
 *
 * This division into four sections is also adopted in other frameworks specialized
 * in parallel computation over meshes, such as OP2 and PyOP2.
 */
inline set_t* set (std::string name, int size, int owned = 0, int execHalo = 0,
                   int nonExecHalo = 0, bool isSubset = false)
{
  set_t* set =  new set_t;
  set->name = name;
  set->size = size;
  set->owned = owned;
  set->execHalo = execHalo;
  set->nonExecHalo = nonExecHalo;
  set->isSubset = isSubset;
  return set;
}

/*
 * Copy a set
 */
inline set_t* set_cpy (set_t* toCopy)
{
  return set(toCopy->name, toCopy->size, toCopy->owned, toCopy->execHalo,
             toCopy->nonExecHalo, toCopy->isSubset);
}

/*
 * Compare two sets based on their name identifier
 */
inline bool set_cmp(const set_t* a, const set_t* b)
{
  return a->name < b->name;
}

/*
 * Destroy a set
 */
inline void set_free (set_t* set)
{
  if (! set) {
    return;
  }
  delete set;
}

#endif
