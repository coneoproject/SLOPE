/*
 * set.h
 *
 */

#ifndef _SET_H_
#define _SET_H_

#include <set>

#include <string>
#include <stdlib.h>
#include <string.h>

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
  /* core and export halos */
  int setSize;
  /* multiple core sizes for multiple halos */
  int* coreSizes;
  /* multiple exec sizes for multiple halos */
  int* execSizes;
  /* multiple nonexec sizes for multiple halos */
  int* nonExecSizes;
  /* current number of halos */
  int curHaloLevel;
  /* maximum number of halos */
  int maxHaloLevel;
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

inline set_t* slop_set (std::string name,
                   int setSize,
                   int* coreSizes = NULL,
                   int* execSizes = NULL,
                   int* nonExecSizes = NULL,
                   int maxHaloLevel = 1,
                   int curHaloLevel = 1,
                   set_t* superset = NULL)
{
  set_t* set =  new set_t;
  set->name = name;
  set->setSize = setSize;
  set->maxHaloLevel = maxHaloLevel;
  set->curHaloLevel = curHaloLevel;

  set->coreSizes = (int*)malloc(maxHaloLevel * sizeof(int));
  memcpy(set->coreSizes, coreSizes, sizeof(int) * maxHaloLevel);
  set->execSizes = (int*)malloc(maxHaloLevel * sizeof(int));
  memcpy(set->execSizes, execSizes, sizeof(int) * maxHaloLevel);
  set->nonExecSizes = (int*)malloc(maxHaloLevel * sizeof(int));
  memcpy(set->nonExecSizes, nonExecSizes, sizeof(int) * maxHaloLevel);

  set->core = coreSizes[curHaloLevel - 1];
  set->execHalo = set->setSize - coreSizes[curHaloLevel - 1] + execSizes[curHaloLevel - 1];
  set->nonExecHalo = nonExecSizes[curHaloLevel - 1];
  set->size = set->core + set->execHalo + set->nonExecHalo;
  set->superset = superset;
  return set;
}

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
