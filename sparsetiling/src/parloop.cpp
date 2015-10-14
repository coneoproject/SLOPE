/*
 *  parloop.cpp
 *
 */

#include "parloop.h"

bool loop_load_seed_map (loop_t* loop, loop_list* loops)
{

  // search the map among the access descriptors ...
  bool found = false;
  desc_list::const_iterator it, end;
  desc_list* descriptors = loop->descriptors;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    map_t* map = (*it)->map;
    if (map == DIRECT || map->outSet->superset) {
      continue;
    }
    loop->seedMap = map;
    found = true;
  }

  // ... if not found, try within the other loops
  if (found || !loops) {
      return found;
  }
  loop_list::const_iterator lIt, lEnd;
  for (lIt = loops->begin(), lEnd = loops->end(); lIt != lEnd; lIt++) {
    descriptors = (*lIt)->descriptors;
    for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
      map_t* map = (*it)->map;
      if (map == DIRECT || set_eq(map->outSet, loop->set) || set_eq(map->inSet, loop->set)) {
        continue;
      }
      loop->seedMap = map;
      found = true;
      break;
    }
  }
  return found;
}

bool loop_is_direct (loop_t* loop)
{
  desc_list* descriptors = loop->descriptors;
  desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    if ((*it)->map != DIRECT) {
      return false;
    }
  }
  return true;
}
