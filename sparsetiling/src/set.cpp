/*
 * set.cpp
 *
 */

#include "set.h"

set_t* set (std::string name,
                   int core,
                   int execHalo,
                   int nonExecHalo,
                   set_t* superset)
{
  set_t* set =  new set_t;
  set->name = name;
  set->core = core;
  set->execHalo = execHalo;
  set->nonExecHalo = nonExecHalo;
  set->size = core + execHalo + nonExecHalo;
  set->superset = superset;

  return set;
}

set_t* set_f (const char* name,
                   int core,
                   int execHalo,
                   int nonExecHalo,
                   set_t* superset)
{
    return set(std::string(name), core, execHalo, nonExecHalo, superset);
}

set_t* slop_set (std::string name,
                   int setSize,
                   int* coreSizes,
                   int* execSizes,
                   int* nonExecSizes,
                   int maxHaloLevel,
                   int curHaloLevel,
                   set_t* superset)
{
  set_t* set =  new set_t;
  set->name = name;
  set->setSize = setSize;
  set->maxHaloLevel = maxHaloLevel;
  set->curHaloLevel = curHaloLevel;

  set->coreSizes = (int*)malloc(maxHaloLevel * sizeof(int));
  memcpy(set->coreSizes, coreSizes, sizeof(int) * maxHaloLevel);
  set->execSizes = (int*)malloc(maxHaloLevel * sizeof(int));
  memcpy(set->execSizes, execSizes, sizeof(int) * maxHaloLevel);
  set->nonExecSizes = (int*)malloc(maxHaloLevel * sizeof(int));
  memcpy(set->nonExecSizes, nonExecSizes, sizeof(int) * maxHaloLevel);

  set->core = coreSizes[curHaloLevel - 1];
  set->execHalo = set->setSize - coreSizes[curHaloLevel - 1] + execSizes[curHaloLevel - 1];
  set->nonExecHalo = nonExecSizes[curHaloLevel - 1];
  set->size = set->core + set->execHalo + set->nonExecHalo;
  set->superset = superset;
  return set;
}

set_t* slop_set_f (const char* name,
                   int setSize,
                   int* coreSizes,
                   int* execSizes,
                   int* nonExecSizes,
                   int maxHaloLevel,
                   int curHaloLevel,
                   set_t* superset)
{
  return slop_set(std::string(name), setSize, coreSizes, execSizes, nonExecSizes, maxHaloLevel, curHaloLevel, superset);
}

set_t* set_super(set_t* set)
{
  return (set_t*)set->superset;
}

set_t* set_cpy (set_t* toCopy)
{
  return set(toCopy->name, toCopy->core, toCopy->execHalo, toCopy->nonExecHalo,
             set_super(toCopy));
}

bool set_eq(const set_t* a,
                   const set_t* b)
{
  return a && b && a->name == b->name;
}

bool set_cmp(const set_t* a,
                    const set_t* b)
{
  return a && b && a->name < b->name;
}

void set_free (set_t* set)
{
  if (! set) {
    return;
  }
  delete set;
}