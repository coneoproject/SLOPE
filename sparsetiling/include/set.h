/*
 * set.h
 *
 */

#ifndef _SET_H_
#define _SET_H_

#include <string.h>

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
 * Initialize a set
 */
inline set_t* set (char* setName, int size)
{
  set_t* set = (set_t*) malloc (sizeof(set_t));
  set->setName = setName;
  set->size = size;
  return set;
}

/*
 * Compare two sets based on their name identifier
 */
inline bool set_cmp(const set_t* a, const set_t* b)
{
  return strcmp(a->setName, b->setName) < 0;
}

/*
 * Destroy a set
 */
inline void set_free (set_t* set)
{
  free(set);
}

#endif
