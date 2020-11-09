/*
 * descriptor.cpp
 *
 */

#include "descriptor.h"

descriptor_t* desc (map_t* map,
                           am_t mode)
{
  descriptor_t* desc = new descriptor_t;
  desc->map = map;
  desc->mode = mode;
  return desc;
}

descriptor_t* desc_f (map_t* map,
                           int mode)
{
  return desc(map, static_cast<am_t>(mode));
}

desc_list* desc_list_f()
{
    return new desc_list;
}

desc_list* insert_descriptor_to_f(desc_list* desc_list, descriptor_t* desc)
{
    desc_list->insert(desc);
    return desc_list;
}

void desc_free (descriptor_t* desc)
{
  map_free (desc->map);
  delete desc;
}
