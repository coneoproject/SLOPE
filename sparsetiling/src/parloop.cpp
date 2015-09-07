/*
 *  parloop.cpp
 *
 */

#include "parloop.h"

bool loop_load_full_map (loop_t* loop)
{
  // aliases
  desc_list* descriptors = loop->descriptors;

  bool found = false;
  desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    map_t* map = (*it)->map;
    if (map == DIRECT || map->outSet->superset) {
      continue;
    }
    loop->seedMap = map;
    found = true;
  }
  return found;
}
