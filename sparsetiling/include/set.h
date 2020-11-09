/*
 * set.h
 *
 */

#ifndef _SET_H_
#define _SET_H_

#include <set>

#include <string>

#ifdef __cplusplus
  extern"C" {
#endif

/*
 * Represent a set
 */
typedef struct {
  /* identifier name of the set */
  std::string name;
  /* size of the local set not touching halo regions */
  int core;
  /* size of the halo region (will be executed over redundantly) */
  int execHalo;
  /* size of the halo region that will only be read */
  int nonExecHalo;
  /* size of the whole iteration space, including halo */
  int size;
  /* am I a subset? If so (!= NULL), what is my superset? */
  void* superset;
} set_t;

typedef std::set<set_t*> set_list;

/*
 * Initialize a set
 *
 * If any of {execHalo, nonExecHalo} are not specified, sequential execution
 * is assumed. Otherwise, the logical division into set elements is: ::
 *
 *     [0, core)
 *     [core, execHalo)
 *     [execHalo, nonExecHalo).
 *
 * Where:
 *
 *    - `core`: all of the owned set elements not touching the halo region
 *    - `execHalo`: off-processor set elements plus owned set elements touching \
 *                  off-processor set elements. The extent of this region \
 *                  this region should be proportial to the depth of tiling
 *    - `nonExecHalo`: read when executing the halo region
 */
set_t* set (std::string name,
                   int core,
                   int execHalo = 0,
                   int nonExecHalo = 0,
                   set_t* superset = NULL);

set_t* set_f (const char* name,
                   int core,
                   int execHalo = 0,
                   int nonExecHalo = 0,
                   set_t* superset = NULL);

/*
 * Get the superset, if any
 */
set_t* set_super(set_t* set);

/*
 * Copy a set
 */
set_t* set_cpy (set_t* toCopy);

/*
 * Return /true/ if two sets are identical (same identifier), /false/ otherwise
 */
bool set_eq(const set_t* a,
                   const set_t* b);
 /*
  * Return /true/ if /a/ goes before /b/, /false/ otherwise. This just boils down
  * to compare the name of the two sets
  */
bool set_cmp(const set_t* a,
                    const set_t* b);
/*
 * Destroy a set
 */
void set_free (set_t* set);

#ifdef __cplusplus
  }
#endif
#endif
