/*
 *  descriptor.h
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

#include "map.h"

/* Define the ways a set can be accessed */
enum am_t {READ, WRITE, RW, INC};

/*
 * Represent an access descriptor, which binds two things:
 * - a map from the iteration set of a parloop to a target set
 * - the way the target set is accessed (READ, WRITE, RW, INC)
 */
typedef struct {
  /* map used to access a certain set */
  map_t* map;
  /* access mode */
  am_t mode;
} descriptor_t;

typedef std::list<descriptor_t*> desc_list;

/*
 * Initialize an access descriptor
 */
inline descriptor_t* desc (map_t* map, am_t mode)
{
  descriptor_t* desc = new descriptor_t;
  desc->map = map;
  desc->mode = mode;
  return desc;
}

/*
 * Destroy an access descriptor
 */
inline void desc_free (descriptor_t* desc)
{
  delete desc;
}

#endif
