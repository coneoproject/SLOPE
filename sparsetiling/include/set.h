/*
 * set.h
 *
 */

#ifndef _SET_H_
#define _SET_H_

#include <string>

/*
 * Represent a set
 */
typedef struct {
  /* identifier name of the set */
  std::string setName;
  /* size of the set */
  int size;
} set_t;

/*
 * Initialize a set
 */
inline set_t* set (std::string setName, int size)
{
  set_t* set =  new set_t;
  set->setName = setName;
  set->size = size;
  return set;
}

/*
 * Copy a set
 */
inline set_t* set_cpy (set_t* toCopy)
{
  set_t* set =  new set_t;
  set->setName = toCopy->setName;
  set->size = toCopy->size;
  return set;
}

/*
 * Compare two sets based on their name identifier
 */
inline bool set_cmp(const set_t* a, const set_t* b)
{
  return a->setName < b->setName;
}

/*
 * Destroy a set
 */
inline void set_free (set_t* set)
{
  if (! set) {
    return;
  }
  delete set;
}

#endif
