/*
 *  parloop.cpp
 *
 */

#include "parloop.h"

bool loop_load_seed_map (loop_t* loop, loop_list* loops)
{

  // search the map among the access descriptors ...
  desc_list::const_iterator it, end;
  desc_list* descriptors = loop->descriptors;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    map_t* map = (*it)->map;
    if (map == DIRECT || map->outSet->superset) {
      continue;
    }
    loop->seedMap = map;
    return true;
  }

  // ... if not found, try within the other loops
  if (!loops) {
      return false;
  }
  loop_list::const_iterator lIt, lEnd;
  // 1) search for a map;
  for (lIt = loops->begin(), lEnd = loops->end(); lIt != lEnd; lIt++) {
    descriptors = (*lIt)->descriptors;
    for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
      map_t* map = (*it)->map;
      if (map != DIRECT && set_eq(loop->set, map->inSet)) {
        loop->seedMap = map;
        return true;
      }
    }
  }
  // 2) if not found, try to see if at least a reverse map is available
  for (lIt = loops->begin(), lEnd = loops->end(); lIt != lEnd; lIt++) {
    descriptors = (*lIt)->descriptors;
    for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
      map_t* map = (*it)->map;
      if (map != DIRECT && set_eq(loop->set, map->outSet)) {
        loop->seedMap = map_invert(map, NULL);
        return true;
      }
    }
  }

  return false;
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
