/*
 *  descriptors.h
 *
 * Define data structure and simple functions to handle:
 * - sets
 * - maps
 * - access modes
 */

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

#include <list>

#include <stdlib.h>

/* Define the ways a set can be accessed */
enum am_t {READ, WRITE, RW, INC};

/*
 * Represent a set
 */
typedef struct {
  /* identifier name of the set */
  char* setName;
  /* size of the set */
  int size;
} set_t;

/*
 * Represent a mapping
 */
typedef struct {
  /* input iteration set */
  set_t* inSet;
  /* output iteration set */
  set_t* outSet;
  /* indirect map from input to output iteration sets */
  int* indMap;
  /* size of indMap */
  int mapSize;
} map_t;

/*
 * Represent an access descriptor, which binds three things:
 * - a set (represented as a string)
 * - the map used to access the set
 * - the way the set is accessed (READ, WRITE, RW, INC)
 */
typedef struct {
  /* map used to access a certain set */
  map_t* map;
  /* access mode */
  am_t mode;
} descriptor_t;

typedef std::list<descriptor_t*> desc_list;

#define DIRECT NULL

/*
 * Initialize a map
 */
inline map_t* map(set_t* inSet, set_t* outSet, int* indMap, int mapSize)
{
  map_t* map = (map_t*) malloc (sizeof(map_t));
  map->inSet = inSet;
  map->outSet = outSet;
  map->indMap = indMap;
  map->mapSize = mapSize;
  return map;
}

/*
 * Destroy a map
 */
inline void map_free(map_t* map)
{
  free(map);
}

/*
 * Initialize an access descriptor
 */
inline descriptor_t* desc(map_t* map, am_t mode)
{
  descriptor_t* desc = (descriptor_t*) malloc (sizeof(descriptor_t));
  desc->map = map;
  desc->mode = mode;
  return desc;
}

/*
 * Destroy an access descriptor
 */
inline void map_free(descriptor_t* desc)
{
  free(desc);
}

/*
 * Initialize a set
 */
inline set_t* set(char* setName, int size)
{
  set_t* set = (set_t*) malloc (sizeof(set_t));
  set->setName = setName;
  set->size = size;
  return set;
}

/*
 * Destroy a set
 */
inline void set_free(set_t* set)
{
  free(set);
}


#endif
