/*
 * schedule.cpp
 *
 * Implementation of a schedule
 */

#include <string.h>

#include "schedule.h"

schedule_t* schedule_init (std::string name, int itSetSize, int* iter2tile,
                           int* iter2color, direction_t direction)
{
  schedule_t* schedule = new schedule_t;

  schedule->name = name;
  schedule->itSetSize = itSetSize;
  schedule->iter2tile = iter2tile;
  schedule->iter2color = iter2color;
  schedule->direction = direction;

  return schedule;
}

schedule_t* schedule_cpy (schedule_t* toCopy)
{
  schedule_t* schedule = new schedule_t;

  schedule->name = toCopy->name;
  schedule->itSetSize = toCopy->itSetSize;
  schedule->iter2tile = new int[toCopy->itSetSize];
  schedule->iter2color = new int[toCopy->itSetSize];
  schedule->direction = toCopy->direction;

  memcpy (schedule->iter2tile, toCopy->iter2tile, sizeof(int)*toCopy->itSetSize);
  memcpy (schedule->iter2color, toCopy->iter2color, sizeof(int)*toCopy->itSetSize);

  return schedule;
}

void schedule_free (schedule_t* schedule)
{
  if (! schedule) {
    return;
  }
  delete[] schedule->iter2tile;
  delete[] schedule->iter2color;
  delete schedule;
}

projection_t* projection_init()
{
  return new projection_t (&schedule_cmp);
}

void projection_free (projection_t* projection)
{
  projection_t::iterator it, end;
  for (it = projection->begin(), end = projection->end(); it != end; it++) {
    schedule_free(*it);
  }
  delete projection;
}
