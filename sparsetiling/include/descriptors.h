/*
 *  descriptors.h
 *
 * Define data structure and simple functions to handle:
 * - sets
 * - datasets
 * - maps
 * - access modes
 */

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

#include <list>

#include <stdlib.h>

/* Define the ways a dataset can be accessed */
enum am_t {READ, WRITE, RW, INC};

/*
 * Represent a mapping
 */
typedef struct {
  /* size of the input iteration set */
  int inSetSize;
  /* size of the output iteration set */
  int outSetSize;
  /* indirect map from input to output iteration sets */
  int* indMap;
  /* size of indMap */
  int mapSize;
} map_t;

/*
 * Represent an access descriptor, which binds three things:
 * - a dataset (represented as a string)
 * - the map used to access the dataset
 * - the way the dataset is accessed (READ, WRITE, RW, INC)
 */
typedef struct {
  /* dataset accessed */
  char* ds;
  /* map used to access ds */
  map_t* map;
  /* access mode */
  am_t mode;
} descriptor_t;

typedef std::list<descriptor_t*> desc_list;

#define DIRECT NULL

/*
 * Initialize a map
 */
inline map_t* map(int inSetSize, int outSetSize, int* indMap, int mapSize)
{
  map_t* map = (map_t*) malloc (sizeof(map_t));
  map->inSetSize = inSetSize;
  map->outSetSize = outSetSize;
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
inline descriptor_t* desc(char* ds, map_t* map, am_t mode)
{
  descriptor_t* desc = (descriptor_t*) malloc (sizeof(descriptor_t));
  desc->ds = ds;
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


#endif
