/*
 *  tiling.cpp
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#include <algorithm>

#include <string.h>
#include <limits.h>

#include "tiling.h"
#include <mpi.h>


inline static void derive_dependency_free_tiling (loop_t* curLoop,
                                                  projection_t* prevLoopProj,
                                                  schedule_t* loopIter2tc);
inline static void update_tiles_tracker (tracker_t& iterTilesPerColor,
                                         index_set iterColors,
                                         tracker_t& conflictsTracker);

void project_forward (loop_t* tiledLoop,
                      schedule_t* tilingInfo,
                      projection_t* prevLoopProj,
                      projection_t* seedLoopProj,
                      tracker_t* conflictsTracker,
                      bool ignoreWAR)
{
  // aliases
  desc_list* descriptors = tiledLoop->descriptors;
  int* iter2tile = tilingInfo->iter2tile;
  int* iter2color = tilingInfo->iter2color;

  bool directHandled = false;
  desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    // aliases
    map_t* descMap = (*it)->map;
    am_t descMode = (*it)->mode;

    schedule_t* projIter2tc;
    if (descMap == DIRECT) {
      if (directHandled) {
        // no need to handle direct descriptors more than once
        continue;
      }
      // direct set case: just replicate the current tiling
      // note that multiple DIRECT descriptors have no effect on the projection,
      // since a requirement for a projection is that its elements are unique
      projIter2tc = tilingInfo;
      directHandled = true;
    }
    else {
      // indirect set case

      // is there anything to project ?
      if (descMap->inSet->size == 0 || (descMode == READ && ignoreWAR)) {
        continue;
      }

      // use the inverse map to compute the projection for two reasons:
      // - the outer loop along the projected set is fully parallel, so it can
      //   be straightforwardly decorated with a /#pragma omp for/
      // - checking conflicts requires to store only O(k) instead of O(kN) memory,
      //   with k the average arity of a projected set iteration and N the size of
      //   the projected iteration set
      int rank, size;
      MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      // if(rank == 1){
      //   printf("descMap name=%s inset=%s outset=%s\n", descMap->name.c_str(), descMap->inSet->name.c_str(), descMap->outSet->name.c_str());
      //   PRINT_MAP(descMap);
      //   PRINT_INTARR(descMap->values, 0, descMap->size);
      // }
        
#ifdef OP2
      descMap = map_invert (descMap, NULL, tiledLoop->nhalos);
#else
      descMap = map_invert (descMap, NULL);
#endif
      // if(rank == 1){
      //   printf("descMap inv name=%s\n", descMap->name.c_str());
      //   // PRINT_MAP(descMap);
      //   PRINT_INTARR(descMap->values, 0, descMap->size);
      //   PRINT_INTARR(descMap->offsets, 0, descMap->inSet->size);

      // }
        
      
      // aliases
// #ifdef OP2
//       printf("tiledLoop=%s set=%s nhalos=%d size=%d exec=%d\n", tiledLoop->name.c_str(), )
//       int projSetSize = descMap->inSet->setSize + descMap->inSet->execSizes[tiledLoop->nhalos - 1];
// #else
      int projSetSize = descMap->inSet->size;
// #endif
      std::string projSetName = descMap->inSet->name;
      int* indMap = descMap->values;
      int* offsets = descMap->offsets;

      int* projIter2tile = new int[projSetSize];
      int* projIter2color = new int[projSetSize];
      projIter2tc = schedule_init (projSetName, projSetSize, projIter2tile,
                                   projIter2color, DOWN);

      
      #pragma omp parallel
      {
        tracker_t localTracker;
        // iterate over the projected loop iteration set, and use the map to access
        // the tiledLoop iteration set's elements.
        #pragma omp for schedule(static)
        for (int i = 0; i < projSetSize; i++) {
          projIter2tile[i] = -1;
          projIter2color[i] = -1;
          // determine the projected set iteration arity, which may vary from
          // iteration to iteration
          int prevOffset = offsets[i];
          int nextOffset = offsets[i + 1];
          // temporary variables for updating the tracker
          tracker_t iterTilesPerColor;
          index_set iterColors;
          for (int j = prevOffset; j < nextOffset; j++) {
            int indIter = indMap[j];
            int indTile = iter2tile[indIter];
            int indColor = iter2color[indIter];
            if(rank == 1){
            // printf("j=%d prevOffset=%d nextOffset=%d indIter=%d indTile=%d indColor=%d\n", j, prevOffset, nextOffset, indMap[j], iter2tile[indIter], iter2color[indIter]);
            }
            // may have to change color and tile of the projected iteration
            int maxColor = MAX(projIter2color[i], indColor);
            if (maxColor != projIter2color[i]) {
              projIter2tile[i] = indTile;
              projIter2color[i] = maxColor;
            }
            // track adjacent tiles, stored by colors
            iterTilesPerColor[indColor].insert (indTile);
            iterColors.insert (indColor);
          }
          update_tiles_tracker (iterTilesPerColor, iterColors, localTracker);
        }

        // need to copy back the conflicts detected by a thread into the global structure
        #pragma omp critical
        {
          tracker_t::const_iterator it, end;
          for (it = localTracker.begin(), end = localTracker.end(); it != end; it++) {
            int tileID = it->first;
            index_set localAdjTiles = it->second;
            (*conflictsTracker)[tileID].insert (localAdjTiles.begin(), localAdjTiles.end());
          }
        }
      }

      // if projecting from a subset, an older projection must be present. This
      // is used to replicate an untouched iteration color and tile.
      projection_t::iterator oldProjIter2tc = prevLoopProj->find (projIter2tc);
      if (oldProjIter2tc != prevLoopProj->end()) {
        #pragma omp for schedule(static)
        for (int i = 0; i < projSetSize; i++) {
          if (projIter2tile[i] == -1) {
            projIter2tile[i] = (*oldProjIter2tc)->iter2tile[i];
            projIter2color[i] = (*oldProjIter2tc)->iter2color[i];
          }
        }
      }
      else {
        set_t* superset = set_super(tiledLoop->set);
        set_t* outSuperset = set_super(descMap->outSet);
        ASSERT (! (superset && (! outSuperset || descMap->outSet->size != superset->size)),
                "Need old projection for subsets");
      }

      map_free (descMap, true);
    }

    // update projections:
    // - seedLoopProj is added a projection for a set X if X is not in seedLoopProj
    //   yet. This is because seedLoopProj will be used for backward tiling, in which
    //   the sets projections closest (in time) to the seed parloop need to be seen
    // - prevLoopProj is updated everytime a new projection is available; for this,
    //   any previous projections for a same set are deleted and memory is freed
    // Note: if the projection still has to be added to seedLoopProj, then for sure it
    //       is not in prevLoopProj either. On the other hand, if the projection is
    //       in seedLoopProj, then a projection on the same set is in prevLoopProj
    if (seedLoopProj->find (projIter2tc) == seedLoopProj->end()) {
      seedLoopProj->insert (schedule_cpy(projIter2tc));
    }
    else {
      projection_t::iterator toFree = prevLoopProj->find (projIter2tc);
      if (toFree != prevLoopProj->end()) {
        schedule_free (*toFree);
        prevLoopProj->erase (toFree);
      }
    }
    prevLoopProj->insert (projIter2tc);
  }

  // can free a tiling function if unused later on
  if (! directHandled) {
    schedule_free (tilingInfo);
  }
}

void project_backward (loop_t* tiledLoop,
                       schedule_t* tilingInfo,
                       projection_t* prevLoopProj,
                       tracker_t* conflictsTracker,
                       bool ignoreWAR)
{
  // aliases
  desc_list* descriptors = tiledLoop->descriptors;
  int* iter2tile = tilingInfo->iter2tile;
  int* iter2color = tilingInfo->iter2color;

  bool directHandled = false;
  desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    // aliases
    map_t* descMap = (*it)->map;
    am_t descMode = (*it)->mode;

    schedule_t* projIter2tc;
    if (descMap == DIRECT) {
      if (directHandled) {
        // no need to handle direct descriptors more than once
        continue;
      }
      // direct set case: just replicate the current tiling
      // note that multiple DIRECT descriptors have no effect on the projection,
      // since a requirement for a projection is that its elements are unique
      projIter2tc = tilingInfo;
      directHandled = true;
    }
    else {
      // indirect set case

      // is there anything to project ?
      if (descMap->inSet->size == 0 || (descMode == READ && ignoreWAR)) {
        continue;
      }

      // use the inverse map to compute the projection for two reasons:
      // - the outer loop along the projected set is fully parallel, so it can
      //   be straightforwardly decorated with a /#pragma omp for/
      // - checking conflicts requires to store only O(k) instead of O(kN) memory,
      //   with k the average arity of a projected set iteration and N the size of
      //   the projected iteration set
#ifdef OP2
      descMap = map_invert (descMap, NULL, tiledLoop->nhalos);
#else
      descMap = map_invert (descMap, NULL);
#endif

// #ifdef OP2
//       int projSetSize = descMap->inSet->setSize + descMap->inSet->execSizes[tiledLoop->nhalos - 1];
// #else
      // aliases
      int projSetSize = descMap->inSet->size;
// #endif
      std::string projSetName = descMap->inSet->name;
      int* indMap = descMap->values;
      int* offsets = descMap->offsets;

      int* projIter2tile = new int[projSetSize];
      int* projIter2color = new int[projSetSize];
      projIter2tc = schedule_init (projSetName, projSetSize, projIter2tile,
                                   projIter2color, UP);

      #pragma omp parallel
      {
        tracker_t localTracker;
        // iterate over the projected loop iteration set, and use the map to access
        // the tiledLoop iteration set's elements.
        #pragma omp for schedule(static)
        for (int i = 0; i < projSetSize; i++) {
          projIter2tile[i] = INT_MAX;
          projIter2color[i] = INT_MAX;
          // determine the projected set iteration arity, which may vary from
          // iteration to iteration
          int prevOffset = offsets[i];
          int nextOffset = offsets[i + 1];
          // temporary variables for updating the tracker
          tracker_t iterTilesPerColor;
          index_set iterColors;
          for (int j = prevOffset; j < nextOffset; j++) {
            int indIter = indMap[j];
            int indTile = iter2tile[indIter];
            int indColor = iter2color[indIter];
            // may have to change color and tile of the projected iteration
            int minColor = MIN(projIter2color[i], indColor);
            if (minColor != projIter2color[i]) {
              projIter2tile[i] = indTile;
              projIter2color[i] = minColor;
            }
            // track adjacent tiles, stored by colors
            iterTilesPerColor[indColor].insert (indTile);
            iterColors.insert (indColor);
          }
          update_tiles_tracker (iterTilesPerColor, iterColors, localTracker);
        }

        // need to copy back the conflicts detected by a thread into the global structure
        #pragma omp critical
        {
          tracker_t::const_iterator it, end;
          for (it = localTracker.begin(), end = localTracker.end(); it != end; it++) {
            int tileID = it->first;
            index_set localAdjTiles = it->second;
            (*conflictsTracker)[tileID].insert (localAdjTiles.begin(), localAdjTiles.end());
          }
        }
      }

      // if projecting from a subset, an older projection must be present. This
      // is used to replicate an untouched iteration color and tile.
      projection_t::iterator oldProjIter2tc = prevLoopProj->find (projIter2tc);
      if (oldProjIter2tc != prevLoopProj->end()) {
        #pragma omp for schedule(static)
        for (int i = 0; i < projSetSize; i++) {
          if (projIter2tile[i] == INT_MAX) {
            projIter2tile[i] = (*oldProjIter2tc)->iter2tile[i];
            projIter2color[i] = (*oldProjIter2tc)->iter2color[i];
          }
        }
      }
      else {
        set_t* superset = set_super(tiledLoop->set);
        set_t* outSuperset = set_super(descMap->outSet);
        ASSERT (! (superset && (! outSuperset || descMap->outSet->size != superset->size)),
                "Need old projection for subsets");
      }

      map_free (descMap, true);
    }

    // update projections:
    // - prevLoopProj is updated everytime a new projection is available; for this,
    //   any previous projections for a same set are deleted and memory is freed
    projection_t::iterator toFree = prevLoopProj->find (projIter2tc);
    if (toFree != prevLoopProj->end()) {
      schedule_free (*toFree);
      prevLoopProj->erase (toFree);
    }
    prevLoopProj->insert (projIter2tc);
  }

  // can free a tiling function if unused later on
  if (! directHandled) {
    schedule_free (tilingInfo);
  }
}

schedule_t* tile_forward (loop_t* curLoop,
                          projection_t* prevLoopProj)
{
  // aliases
  set_t* toTile = curLoop->set;
#ifdef OP2
  int toTileSetSize = toTile->setSize + toTile->execSizes[curLoop->nhalos - 1];
#else
  int toTileSetSize = toTile->size;
#endif
  std::string toTileSetName = toTile->name;
  desc_list* descriptors = curLoop->descriptors;
  schedule_t *loopIter2tc;

  // the following contains all projected iteration sets that have already been
  // used to determine a tiling and a coloring for curLoop
  std::set<set_t*, bool(*)(const set_t* a, const set_t* b)> checkedSets (&set_cmp);

  // allocate and initialize space to keep tiling and coloring results
  int* loopIter2tile = new int[toTileSetSize];
  int* loopIter2color = new int[toTileSetSize];
  std::fill_n (loopIter2tile, toTileSetSize, -1);
  std::fill_n (loopIter2color, toTileSetSize, -1);
  loopIter2tc = schedule_init (toTileSetName, toTileSetSize, loopIter2tile,
                               loopIter2color, DOWN);

  
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if(rank == 0){
    // printf("loopIter2tile toTileSetSize=%d <<<<<<>>>>>>\n", toTileSetSize);
    // PRINT_INTARR(loopIter2tile, 0, toTileSetSize);
  }

  if (toTileSetSize == 0) {
    // no need to tile
    return loopIter2tc;
  }

  desc_list::const_iterator it, end;
  if(rank == 0){
    printf("============================start ===================\n");
  }
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    // aliases
    map_t* descMap = (*it)->map;
    am_t descMode = (*it)->mode;
    set_t* touchedSet = (descMap == DIRECT) ? toTile : descMap->outSet;
    std::string touchedSetName = touchedSet->name;

    if (checkedSets.find(touchedSet) != checkedSets.end()) {
      // set already used for computing a tiling of curLoop (e.g. it was found
      // in a previous descriptor), so skip recomputing same information
      continue;
    }

    // retrieve projected iteration set to tile-color info
    // if no projection is present, just go on to the next descriptor because it
    // means that the touched set, regardless of whether it is touched directly
    // or indirectly, does not introduce any dependency with the loop being tiled
    schedule_t projIter2tc = {touchedSetName};
    projection_t::iterator iprojIter2tc = prevLoopProj->find (&projIter2tc);
    if (iprojIter2tc == prevLoopProj->end()) {
      continue;
    }
    int* projIter2tile = (*iprojIter2tc)->iter2tile;
    int* projIter2color = (*iprojIter2tc)->iter2color;
    if(rank == 0){
      // PRINT_PROJECTION(prevLoopProj, curLoop);
      // printf("tiling projIter2tile curLoop->set=%s(size=%d) size=%d\n", curLoop->set->name.c_str(), curLoop->set->size, toTileSetSize);
      // PRINT_INTARR(projIter2tile, 0, toTileSetSize);
    }

    if (touchedSet == toTile) {
      // direct set case
      #pragma omp parallel for schedule(static)
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = projIter2tile[i];
        int iterColor = MAX(projIter2color[i], loopIter2color[i]);
        // if(rank == 0){
        //   printf("test>>> i=[%d] projiter=%d loopiter=%d\n", i, projIter2tile[i], loopIter2tile[i]);
        // }
        if (iterColor != loopIter2color[i]) {
          loopIter2tile[i] = iterTile;
          loopIter2color[i] = iterColor;
        }
        
      }
    }
    else {
      // indirect set case
      // aliases
      int touchedSetSize = touchedSet->size;
      int mapSize = descMap->size;
      int* indMap = descMap->values;

#ifdef OP2
      int arity = (descMap->dim < 0) ? mapSize / toTileSetSize : descMap->dim;
#else
      int arity = mapSize / toTileSetSize;
#endif
    if(rank == 0){
      // printf("tiling map=%s in=%s(size=%d) out=%s(size=%d) mapSize=%d toTileSetSize=%d arity=%d dim=%d\n", descMap->name.c_str(), descMap->inSet->name.c_str(), descMap->inSet->size, descMap->outSet->name.c_str(), descMap->outSet->size, mapSize, toTileSetSize, arity, descMap->dim);
    }

      // iterate over the iteration set of the loop we are tiling, and use the map
      // to access the indirectly touched elements
      #pragma omp parallel for schedule(static)
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = loopIter2tile[i];
        int iterColor = loopIter2color[i];
        for (int j = 0; j < arity; j++) {
          int indIter = indMap[i*arity + j];
          if (indIter == -1) {
            // off-processor elements are set to -1; ignore them
            continue;
          }
          

          int indTile = projIter2tile[indIter];
          if(rank == 0){
            // printf("afterloopIter2tile i=%d toTileSetSize=%d iterTile=%d arity=%d j=%d indMap[%d]=%d indTile=%d\n", i, toTileSetSize, iterTile, arity, i, i*arity + j, indMap[i*arity + j], indTile);
          }
          int indColor = MAX(iterColor, projIter2color[indIter]);
          //  if(rank == 0){
          //   printf("test>>> i=[%d] j=%d projiter=%d loopiter=%d\n", i, j, projIter2tile[i], loopIter2tile[i]);
          // }
          if (iterColor != indColor) {
            // update color and tile of the loop being tiled
            iterTile = indTile;
            iterColor = indColor;
          }
        }
        // now all adjacent iterations have been examined, so assign the MAX
        // color found and the corresponding tile
        loopIter2tile[i] = iterTile;
        loopIter2color[i] = iterColor;
        // if(rank == 0){
        //   printf("afterloopIter2tile loopIter2tile[%d]=%d\n", i, loopIter2tile[i]);
        // }
      }
    }

    // mark the schedule as performed
    loopIter2tc->computed = true;

    checkedSets.insert (touchedSet);
  }

  if(rank == 0){
    printf("============================end ===================\n");
  }

  if(rank == 0){
    // printf("afterloopIter2tile toTileSetSize=%d <<<<<<>>>>>>\n", toTileSetSize);
    // PRINT_INTARR(loopIter2tile, 0, toTileSetSize);
  }

  // if no schedule could be computed (as, for example, the iteration set of
  // /curLoop/ is encountered for the first time), try deriving a sensible tiling
  // from any of the previously tiled loops
  if (! loopIter2tc->computed) {
    derive_dependency_free_tiling (curLoop, prevLoopProj, loopIter2tc);
    loopIter2tc->computed = true;
  }

#ifdef SLOPE_VTK
  // track coloring and tiling of a parloop. These can be used for debugging or
  // visualization purpose, e.g. for generating VTK files.
  curLoop->tiling = new int[toTileSetSize];
  curLoop->coloring = new int[toTileSetSize];
  memcpy (curLoop->tiling, loopIter2tc->iter2tile, sizeof(int)*toTileSetSize);
  memcpy (curLoop->coloring, loopIter2tc->iter2color, sizeof(int)*toTileSetSize);
#endif

  return loopIter2tc;
}

schedule_t* tile_backward (loop_t* curLoop,
                           projection_t* prevLoopProj)
{
  // aliases
  set_t* toTile = curLoop->set;
#ifdef OP2
  int toTileSetSize = toTile->setSize + toTile->execSizes[curLoop->nhalos - 1];
#else
  int toTileSetSize = toTile->size;
#endif
  std::string toTileSetName = toTile->name;
  desc_list* descriptors = curLoop->descriptors;
  schedule_t *loopIter2tc;

  // the following contains all projected iteration sets that have already been
  // used to determine a tiling and a coloring for curLoop
  std::set<set_t*, bool(*)(const set_t* a, const set_t* b)> checkedSets (&set_cmp);

  // allocate and initialize space to keep tiling and coloring results
  int* loopIter2tile = new int[toTileSetSize];
  int* loopIter2color = new int[toTileSetSize];
  std::fill_n (loopIter2tile, toTileSetSize, INT_MAX);
  std::fill_n (loopIter2color, toTileSetSize, INT_MAX);
  loopIter2tc = schedule_init (toTileSetName, toTileSetSize, loopIter2tile,
                               loopIter2color, UP);

  if (toTileSetSize == 0) {
    // no need to tile
    return loopIter2tc;
  }

  desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    // aliases
    map_t* descMap = (*it)->map;
    am_t descMode = (*it)->mode;
    set_t* touchedSet = (descMap == DIRECT) ? toTile : descMap->outSet;
    std::string touchedSetName = touchedSet->name;

    if (checkedSets.find(touchedSet) != checkedSets.end()) {
      // set already used for computing a tiling of curLoop (e.g. it was found
      // in a previous descriptor), so skip recomputing same information
      continue;
    }

    // retrieve projected iteration set to tile-color info
    // if no projection is present, just go on to the next descriptor because it
    // means that the touched set, regardless of whether it is touched directly
    // or indirectly, does not introduce any dependency with the loop being tiled
    schedule_t projIter2tc = {touchedSetName};
    projection_t::iterator iprojIter2tc = prevLoopProj->find (&projIter2tc);
    if (iprojIter2tc == prevLoopProj->end()) {
      continue;
    }
    int* projIter2tile = (*iprojIter2tc)->iter2tile;
    int* projIter2color = (*iprojIter2tc)->iter2color;

    if (touchedSet == toTile) {
      // direct set case
      #pragma omp parallel for schedule(static)
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = projIter2tile[i];
        int iterColor = MIN(projIter2color[i], loopIter2color[i]);
        if (iterColor != loopIter2color[i]) {
          loopIter2tile[i] = iterTile;
          loopIter2color[i] = iterColor;
        }
      }
    }
    else {
      // indirect set case
      // aliases
      int touchedSetSize = touchedSet->size;
      int mapSize = descMap->size;
      int* indMap = descMap->values;

#ifdef OP2
      int arity = (descMap->dim < 0) ? mapSize / toTileSetSize : descMap->dim;
#else
      int arity = mapSize / toTileSetSize;
#endif

      // iterate over the iteration set of the loop we are tiling, and use the map
      // to access the indirectly touched elements
      #pragma omp parallel for schedule(static)
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = loopIter2tile[i];
        int iterColor = loopIter2color[i];
        for (int j = 0; j < arity; j++) {
          int indIter = indMap[i*arity + j];
          if (indIter == -1) {
            // off-processor elements are set to -1; ignore them
            continue;
          }
          int indTile = projIter2tile[indIter];
          int indColor = MIN(iterColor, projIter2color[indIter]);
          if (iterColor != indColor) {
            // update color and tile of the loop being tiled
            iterTile = indTile;
            iterColor = indColor;
          }
        }
        // now all adjacent iterations have been examined, so assign the MIN
        // color found and the corresponding tile
        loopIter2tile[i] = iterTile;
        loopIter2color[i] = iterColor;
      }
    }

    // mark the schedule as performed
    loopIter2tc->computed = true;

    checkedSets.insert (touchedSet);
  }

  // if no schedule could be computed (as, for example, the iteration set of
  // /curLoop/ is encountered for the first time), try deriving a sensible tiling
  // from any of the previously tiled loops
  if (! loopIter2tc->computed) {
    derive_dependency_free_tiling (curLoop, prevLoopProj, loopIter2tc);
    loopIter2tc->computed = true;
  }

#ifdef SLOPE_VTK
  // track coloring and tiling of a parloop. These can be used for debugging or
  // visualization purpose, e.g. for generating VTK files.
  curLoop->tiling = new int[toTileSetSize];
  curLoop->coloring = new int[toTileSetSize];
  memcpy (curLoop->tiling, loopIter2tc->iter2tile, sizeof(int)*toTileSetSize);
  memcpy (curLoop->coloring, loopIter2tc->iter2color, sizeof(int)*toTileSetSize);
#endif

  return loopIter2tc;
}

void assign_loop (loop_t* loop, loop_list* loops, tile_list* tiles,
                  int* iter2tile, direction_t direction)
{
  // aliases
  int loopIndex = loop->index;
  set_t* loopSet = loop->set;
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // 1) remove any previously assigned iteration for loop /loopIndex/
  tile_list::const_iterator tIt, tEnd;
  for (tIt = tiles->begin(), tEnd = tiles->end(); tIt != tEnd; tIt++) {
    (*tIt)->iterations[loopIndex]->clear();
  }

  // 2) distribute iterations to tiles (note: we do not assign non-exec iterations)
  
#ifdef OP2
  int execSize = loopSet->setSize + loopSet->execSizes[loop->nhalos - 1];
#else
  int execSize = loopSet->core + loopSet->execHalo;
#endif
  for (int i = 0; i < execSize; i++) {

    
    if(rank == 0){
      // printf("i=%d loopIndex=%d execsize=%d iter2tile[i]=%d\n", i, loopIndex, execSize, iter2tile[i]);
    }
    tiles->at(iter2tile[i])->iterations[loopIndex]->push_back(i);
  }

  for (tIt = tiles->begin(), tEnd = tiles->end(); tIt != tEnd; tIt++) {
    tile_t* tile = *tIt;
    int nLoops = tile->crossedLoops;
    iterations_list& iterations = *(tile->iterations[loopIndex]);
    int tileLoopSize = iterations.size();
    if (! tileLoopSize) {
      continue;
    }

    // 3) create a sorted iteration list:
    // first put all iterations already in the tile, then all others
    iterations_list sortedIters;
    switch (direction) {
      case SEED:
        break;
      case DOWN:
        for (int i = loopIndex - 1; i >= 0; i--) {
          if (loop_eq_itspace (loop, loops->at(i))) {
            iterations_list& prevIters = *(tile->iterations[i]);
            int tilePrevLoopSize = prevIters.size();
            for (int e = 0; e < tilePrevLoopSize; e++) {
              iterations_list::iterator isIn;
              isIn = std::find (iterations.begin(), iterations.end(), prevIters[e]);
              if (isIn != iterations.end()) {
                sortedIters.push_back(*isIn);
                iterations.erase(isIn);
              }
            }
            break;
          }
        }
        break;
      case UP:
        for (int i = loopIndex + 1; i < nLoops; i++) {
          if (loop_eq_itspace (loop, loops->at(i))) {
            iterations_list& prevIters = *(tile->iterations[i]);
            int tilePrevLoopSize = prevIters.size();
            for (int e = 0; e < tilePrevLoopSize; e++) {
              iterations_list::iterator isIn;
              isIn = std::find (iterations.begin(), iterations.end(), prevIters[e]);
              if (isIn != iterations.end()) {
                sortedIters.push_back(*isIn);
                iterations.erase(isIn);
              }
            }
            break;
          }
        }
        break;
    }
    std::sort (iterations.begin(), iterations.end());
    sortedIters.insert (sortedIters.end(), iterations.begin(), iterations.end());
    iterations.assign (sortedIters.begin(), sortedIters.end());

    // 4) add fake /d/ extra elements in case one wants to prefetch iterations
    // /i/, /i+1/, ..., /i+d/, before having executed iteration /i/
    for (int i = 0; i < tile->prefetchHalo; i++) {
      iterations.push_back(iterations.back());
    }
  }
}


/***** Static / utility functions *****/

inline static void update_tiles_tracker (tracker_t& iterTilesPerColor,
                                         index_set iterColors,
                                         tracker_t& conflictsTracker)
{
  tracker_t::const_iterator it, end;
  for (it = iterTilesPerColor.begin(), end = iterTilesPerColor.end(); it != end; it++) {
    index_set adjTiles = it->second;
    index_set::const_iterator tIt, tEnd;
    for (tIt = adjTiles.begin(), tEnd = adjTiles.end(); tIt != tEnd; tIt++) {
      // if conflicts detected on a color add the relevant information to tiles
      // involved in the conflict
      if (adjTiles.size() > 1) {
        conflictsTracker[*tIt].insert (adjTiles.begin(), adjTiles.end());
      }
    }
    for (tIt = adjTiles.begin(), tEnd = adjTiles.end(); tIt != tEnd; tIt++) {
      conflictsTracker[*tIt].erase (*tIt);
    }
  }
}

inline static void derive_dependency_free_tiling (loop_t* curLoop,
                                                  projection_t* prevLoopProj,
                                                  schedule_t* loopIter2tc)
{
  // aliases
  int toTileSetSize = loopIter2tc->itSetSize;
  int* loopIter2tile = loopIter2tc->iter2tile;
  int* loopIter2color = loopIter2tc->iter2color;
  map_t* indMap = curLoop->seedMap;
  bool fallback = true;

  if (indMap) {
    // try applying the schedule of the indirect set, if it's already been tiled
    schedule_t projIter2tc = {indMap->outSet->name};
    projection_t::iterator iprojIter2tc = prevLoopProj->find (&projIter2tc);
    if (iprojIter2tc != prevLoopProj->end()) {
      int indSetSize = (*iprojIter2tc)->itSetSize;
      int* indIter2tile = (*iprojIter2tc)->iter2tile;
      int* indIter2color = (*iprojIter2tc)->iter2color;

      int maxSize = MIN(toTileSetSize, indSetSize);
      memcpy (loopIter2tile, indIter2tile, sizeof(int)*maxSize);
      memcpy (loopIter2color, indIter2color, sizeof(int)*maxSize);
      // remainder, if necessary
      for (int i = indSetSize; i < toTileSetSize; i++) {
        loopIter2tile[i] = loopIter2tile[i-1];
        loopIter2color[i] = loopIter2color[i-1];
      }
      fallback = false;
    }
  }

  if (fallback) {
    // the /curLoop/ iteration space is never accessed directly and this is the
    // first time it's encountered; readapt one of the known schedules
    std::fill_n (loopIter2tile, toTileSetSize, 0);
    std::fill_n (loopIter2color, toTileSetSize, 0);
  }

}
