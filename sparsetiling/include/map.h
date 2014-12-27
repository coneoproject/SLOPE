/*
 * map.h
 *
 */

#ifndef _MAP_H_
#define _MAP_H_

#include <set>

#include "set.h"

/*
 * Represent a map between two sets
 */
typedef struct {
  /* name/identifier of the map */
  std::string name;
  /* input iteration set */
  set_t* inSet;
  /* output iteration set */
  set_t* outSet;
  /* indirect map from input to output iteration sets */
  int* indMap;
  /* size of indMap (== -1 if an irregular map, in which case the actual size
   * is given by offsets[outSet->size]) */
  int mapSize;
  /* offsets in indMap (!= NULL only if an irregular map) */
  int* offsets;
} map_t;

typedef std::set<map_t*> map_list;

/*
 * Identify direct maps
 */
#define DIRECT NULL

/*
 * Initialize a map
 */
map_t* map (std::string name, set_t* inSet, set_t* outSet, int* indMap, int ariety);

/*
 * Initialize an irregular maps, in which each input entry is mapped to a
 * variable number of output entries. The offsets track the distance between
 * two different output entries in indMap.
 */
map_t* imap (std::string name, set_t* inSet, set_t* outSet, int* indMap, int* offsets);

/*
 * Destroy a map
 */
void map_free (map_t* map, bool freeIndMap = false);

/*
 * Invert a mapping from a set X to a set Y.
 * The only assumption here is that all elements in X are mapped to a same number
 * K of elements in Y.
 *
 * @param x2y
 *   a mapping from a set x to a set y
 * @param maxIncidence
 *   on return: maximum incidence on a y element
 * @return
 *   a mapping from set y to set x
 */
map_t* map_invert (map_t* x2y, int* maxIncidence);

#endif
