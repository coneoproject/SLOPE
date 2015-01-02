/*
 * set.h
 *
 */

#ifndef _SET_H_
#define _SET_H_

#include <set>

#include <string>

/*
 * Represent a set
 */
typedef struct {
  /* identifier name of the set */
  std::string name;
  /* size of the set */
  int size;
  /* subset flag */
  bool isSubset;
} set_t;

typedef std::set<set_t*> set_list;

/*
 * Initialize a set
 */
inline set_t* set (std::string name, int size, bool isSubset = false)
{
  set_t* set =  new set_t;
  set->name = name;
  set->size = size;
  set->isSubset = isSubset;
  return set;
}

/*
 * Copy a set
 */
inline set_t* set_cpy (set_t* toCopy)
{
  set_t* set =  new set_t;
  set->name = toCopy->name;
  set->size = toCopy->size;
  return set;
}

/*
 * Compare two sets based on their name identifier
 */
inline bool set_cmp(const set_t* a, const set_t* b)
{
  return a->name < b->name;
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
