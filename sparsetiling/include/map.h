/*
 * map.h
 *
 */

#ifndef _MAP_H_
#define _MAP_H_

#include <set>

#include "set.h"

#ifdef __cplusplus
  extern"C" {
#endif

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
  int* values;
  /* size of /values/ (== offsets[inSet->size] if an irregular map) */
  int size;
  /* offsets in /values/ when two elements in the input iteration set can have
   * a different number of output iteration set elements. For example, this is
   * the case of a map from vertices to edges in an unstructured mesh. This value
   * is different than NULL only if the map is irregular. The size of this array
   * is inSet->size + 1.*/
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
map_t* map (std::string name,
            set_t* inSet,
            set_t* outSet,
            int* values,
            int size);

map_t* map_f (const char* name,
            set_t* inSet,
            set_t* outSet,
            int* values,
            int size);

/*
 * Return a fresh copy of /map/
 */
map_t* map_cpy (std::string name,
                map_t* map);

/*
 * Initialize an irregular map, in which input entries can be mapped to a
 * varying number of output entries. The offsets track the distance between
 * two different output entries in /values/.
 */
map_t* imap (std::string name,
             set_t* inSet,
             set_t* outSet,
             int* values,
             int* offsets);

/*
 * Destroy a map
 */
void map_free (map_t* map,
               bool freeIndMap = false);

/*
 * Retrieve the offset of /element/ in map.
 *
 * @return
 *   update /offset/ and /size/
 */
void map_ofs (map_t* map,
              int element,
              int* offset,
              int* size);

/*
 * Invert a fixed-arity mapping from a set X to a set Y. This function CANNOT be
 * used for inverting irregular maps.
 *
 * @param x2y
 *   a mapping from a set x to a set y
 * @return
 *   a mapping from set y to set x. Also update /maxIncidence/, which tells the
 *   maximum incidence degree on a y element
 */
map_t* map_invert (map_t* x2y,
                   int* maxIncidence);

map_list* map_list_f();

void insert_map_to_f(map_list* maps, map_t* map);

#ifdef __cplusplus
  }
#endif
#endif
