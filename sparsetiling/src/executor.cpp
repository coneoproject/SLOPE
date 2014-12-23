/*
 *  executor.cpp
 *
 */

#include "executor.h"

executor_t* exec_init (inspector_t* insp)
{
  executor_t* exec = (executor_t*) malloc (sizeof(executor_t));

  tile_list* tiles = insp->tiles;

  exec->tiles = tiles;

  return exec;
}

void exec_free (executor_t* exec)
{
  for (int i = 0; i < exec->tiles->size(); i++) {
    free ((*exec->tiles)[i]);
  }
  delete exec->tiles;
}
