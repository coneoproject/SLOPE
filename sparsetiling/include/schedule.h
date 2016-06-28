/*
 *  tiling.h
 *
 * Scheduling functions map iteration set elements to a tile and a color
 */

#ifndef _SCHEDULE_H_
#define _SCHEDULE_H_

#include <set>
#include <string>

#include "common.h"

typedef struct {
  /* set name identifier */
  std::string name;
  /* iteration set */
  int itSetSize;
  /* tiling of the iteration set tiling */
  int* iter2tile;
  /* coloring of the iteration set coloring */
  int* iter2color;
  /* tiling direction */
  direction_t direction;
  /* has the schedule been computed ? */
  bool computed;

} schedule_t;

/*
 * Map iterations to tile IDs and colors.
 *
 * Note: the caller loses ownership of iter2tile and iter2color after calling
 * this function.
 */
schedule_t* schedule_init (std::string name,
                           int itSetSize,
                           int* iter2tile,
                           int* iter2color,
                           direction_t direction);

/*
 * Clone an schedule_t
 */
schedule_t* schedule_cpy (schedule_t* schedule);

/*
 * Destroy an schedule_t
 */
void schedule_free (schedule_t* schedule);


inline bool schedule_cmp (const schedule_t* a,
                          const schedule_t* b)
{
  return a->name < b->name;
}


/*
 * A loop projection is a collection of scheduling functions
 */
typedef std::set<schedule_t*, bool(*)(const schedule_t* a, const schedule_t* b)> projection_t;

/*
 * Initialize a new loop projection
 */
projection_t* projection_init();

/*
 * Destroy a loop projection
 */
void projection_free (projection_t* projection);

#endif
