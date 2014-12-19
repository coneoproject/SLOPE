/*
 * map.h
 *
 */

#ifndef _MAP_H_
#define _MAP_H_

#include "set.h"

/*
 * Represent a map between two sets
 */
typedef struct {
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

/*
 * Identify direct maps
 */
#define DIRECT NULL

/*
 * Initialize a map
 */
map_t* map (set_t* inSet, set_t* outSet, int* indMap, int mapSize);

/*
 * Initialize an irregular maps, in which each input entry is mapped to a
 * variable number of output entries. The offsets track the distance between
 * two different output entries in indMap.
 */
map_t* imap (set_t* inSet, set_t* outSet, int* indMap, int* offsets);

/*
 * Destroy a map
 */
void map_free (map_t* map);

/*
 * Invert a mapping from a set X to a set Y.
 * The only assumption here is that all elements in X are mapped to a same number
 * K of elements in Y.
 *
 * @param x2y
 *   mapping from set x to set y
 * @param xZero
 *   if xZero == true then the output numbering starts from 0:
 * @param y2x
 *   on return: mapping from y to x
 * @param maxIncidence
 *   on return: maximum incidence on a y element
 */
void map_invert (map_t* x2y, int xZero, map_t* y2x, int* maxIncidence);

#endif
