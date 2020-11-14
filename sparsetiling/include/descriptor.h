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

#include <set>

#include <stdlib.h>

#include "map.h"

#ifdef __cplusplus
  extern"C" {
#endif
/* Define the ways a set can be accessed */
enum am_t {READ, WRITE, RW, INC};

/*
 * Represent an access descriptor, which includes two fields:
 * - a map from the iteration set of a parloop to a target set
 * - the access mode to the target set (READ, WRITE, RW, INC)
 */
typedef struct {
  /* map used to access a certain set */
  map_t* map;
  /* access mode */
  am_t mode;
} descriptor_t;

typedef std::set<descriptor_t*> desc_list;



/*
 * Initialize an access descriptor
 */

descriptor_t* desc (map_t* map,
                           am_t mode);

descriptor_t* desc_f (map_t* map,
                           int mode);

desc_list* desc_list_f();

void insert_descriptor_to_f(desc_list* desc_list, descriptor_t* desc);

/*
 * Destroy an access descriptor
 */
void desc_free (descriptor_t* desc);

#ifdef __cplusplus
  }
#endif
#endif
