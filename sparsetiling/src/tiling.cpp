/*
 *  tiling.cpp
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#include <algorithm>

#include <string.h>
#include <limits.h>

#include "tiling.h"
#include "utils.h"


// return true if there are at least two tiles having a same color
inline static void updateTilesTracker (tracker_t& iterTilesPerColor,
                                       index_set iterColors,
                                       tracker_t& conflictsTracker);

void project_forward (loop_t* tiledLoop,
                      iter2tc_t* tilingInfo,
                      projection_t* prevLoopProj,
                      projection_t* seedLoopProj,
                      tracker_t* conflictsTracker)
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

    iter2tc_t* projIter2tc;
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

      // use the inverted map to compute the projection for two reasons:
      // - the outer loop along the projected set is fully parallel, so it can
      //   be straightforwardly decorated with a /#pragma omp for/
      // - checking conflicts requires to store only O(k) instead of O(kN) memory,
      //   with k the average ariety of a projected set iteration and N the size of
      //   the projected iteration set
      descMap = map_invert (descMap, NULL);

      // aliases
      int projSetSize = descMap->inSet->size;
      std::string projSetName = descMap->inSet->name;
      int* indMap = descMap->values;
      int* offsets = descMap->offsets;

      int* projIter2tile = new int[projSetSize];
      int* projIter2color = new int[projSetSize];
      projIter2tc = iter2tc_init (projSetName, projSetSize, projIter2tile, projIter2color);

      // iterate over the projected loop's iteration set, and use the map to access
      // the tiledLoop iteration set's elements.
      for (int i = 0; i < projSetSize; i++) {
        projIter2tile[i] = -1;
        projIter2color[i] = -1;
        // determine the projected set iteration's ariety, which may vary from
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
          int maxColor = MAX(projIter2color[i], indColor);
          if (maxColor != projIter2color[i]) {
            projIter2tile[i] = indTile;
            projIter2color[i] = maxColor;
          }
          // track adjacent tiles, stored by colors
          iterTilesPerColor[indColor].insert (indTile);
          iterColors.insert (indColor);
        }
        updateTilesTracker (iterTilesPerColor, iterColors, *conflictsTracker);
      }

      // if projecting from a subset, an older projection must be present. This
      // is used to replicate non-touched iterations' color and tile.
      if (tiledLoop->set->isSubset) {
        projection_t::iterator oldProjIter2tc = prevLoopProj->find (projIter2tc);
        ASSERT (oldProjIter2tc != prevLoopProj->end(),
                "Projecting from subset lacks old projection");
        for (int i = 0; i < projSetSize; i++) {
          if (projIter2tile[i] == -1) {
            projIter2tile[i] = (*oldProjIter2tc)->iter2tile[i];
            projIter2color[i] = (*oldProjIter2tc)->iter2color[i];
          }
        }
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
      seedLoopProj->insert (iter2tc_cpy(projIter2tc));
    }
    else {
      projection_t::iterator toFree = prevLoopProj->find (projIter2tc);
      if (toFree != prevLoopProj->end()) {
        iter2tc_free (*toFree);
        prevLoopProj->erase (toFree);
      }
    }
    prevLoopProj->insert (projIter2tc);
  }
}

void project_backward (loop_t* tiledLoop,
                       iter2tc_t* tilingInfo,
                       projection_t* prevLoopProj,
                       tracker_t* conflictsTracker)
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

    iter2tc_t* projIter2tc;
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

      // use the inverted map to compute the projection for two reasons:
      // - the outer loop along the projected set is fully parallel, so it can
      //   be straightforwardly decorated with a /#pragma omp for/
      // - checking conflicts requires to store only O(k) instead of O(kN) memory,
      //   with k the average ariety of a projected set iteration and N the size of
      //   the projected iteration set
      descMap = map_invert (descMap, NULL);

      // aliases
      int projSetSize = descMap->inSet->size;
      std::string projSetName = descMap->inSet->name;
      int* indMap = descMap->values;
      int* offsets = descMap->offsets;

      int* projIter2tile = new int[projSetSize];
      int* projIter2color = new int[projSetSize];
      projIter2tc = iter2tc_init (projSetName, projSetSize, projIter2tile, projIter2color);

      // iterate over the projected loop's iteration set, and use the map to access
      // the tiledLoop iteration set's elements.
      for (int i = 0; i < projSetSize; i++) {
        projIter2tile[i] = INT_MAX;
        projIter2color[i] = INT_MAX;
        // determine the projected set iteration's ariety, which may vary from
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
        updateTilesTracker (iterTilesPerColor, iterColors, *conflictsTracker);
      }

      // if projecting from a subset, an older projection must be present. This
      // is used to replicate non-touched iterations' color and tile.
      if (tiledLoop->set->isSubset) {
        projection_t::iterator oldProjIter2tc = prevLoopProj->find (projIter2tc);
        ASSERT (oldProjIter2tc != prevLoopProj->end(),
                "Projecting from subset lacks old projection");
        for (int i = 0; i < projSetSize; i++) {
          if (projIter2tile[i] == INT_MAX) {
            projIter2tile[i] = (*oldProjIter2tc)->iter2tile[i];
            projIter2color[i] = (*oldProjIter2tc)->iter2color[i];
          }
        }
      }

      map_free (descMap, true);
    }

    // update projections:
    // - prevLoopProj is updated everytime a new projection is available; for this,
    //   any previous projections for a same set are deleted and memory is freed
    projection_t::iterator toFree = prevLoopProj->find (projIter2tc);
    if (toFree != prevLoopProj->end()) {
      iter2tc_free (*toFree);
      prevLoopProj->erase (toFree);
    }
    prevLoopProj->insert (projIter2tc);
  }
}

iter2tc_t* tile_forward (loop_t* curLoop,
                         projection_t* prevLoopProj)
{
  // aliases
  set_t* toTile = curLoop->set;
  int toTileSetSize = toTile->size;
  std::string toTileSetName = toTile->name;
  desc_list* descriptors = curLoop->descriptors;
  iter2tc_t *loopIter2tc;

  // the following contains all projected iteration sets that have already been
  // used to determine a tiling and a coloring for curLoop
  std::set<set_t*, bool(*)(const set_t* a, const set_t* b)> checkedSets (&set_cmp);

  // allocate and initialize space to keep tiling and coloring results
  int* loopIter2tile = new int[toTileSetSize];
  int* loopIter2color = new int[toTileSetSize];
  std::fill_n (loopIter2tile, toTileSetSize, -1);
  std::fill_n (loopIter2color, toTileSetSize, -1);
  loopIter2tc = iter2tc_init (toTileSetName, toTileSetSize, loopIter2tile, loopIter2color);

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
    iter2tc_t projIter2tc = {touchedSetName};
    projection_t::iterator iprojIter2tc = prevLoopProj->find (&projIter2tc);
    if (iprojIter2tc == prevLoopProj->end()) {
      continue;
    }
    int* projIter2tile = (*iprojIter2tc)->iter2tile;
    int* projIter2color = (*iprojIter2tc)->iter2color;

    if (touchedSet == toTile) {
      // direct set case
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = projIter2tile[i];
        int iterColor = MAX(projIter2color[i], loopIter2color[i]);
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

      int ariety = mapSize / toTileSetSize;

      // iterate over the iteration set of the loop we are tiling, and use the map
      // to access the indirectly touched elements
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = loopIter2tile[i];
        int iterColor = loopIter2color[i];
        for (int j = 0; j < ariety; j++) {
          int indIter = indMap[i*ariety + j];
          int indTile = projIter2tile[indIter];
          int indColor = MAX(iterColor, projIter2color[indIter]);
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
      }
    }

    checkedSets.insert (touchedSet);
  }

#ifdef SLOPE_VTK
  // if requested at compile time, the coloring and tiling of a parloop are
  // explicitly tracked. These can be used for debugging or visualization purposes,
  // for example for generating VTK files showing the colored parloop
  curLoop->tiling = new int[toTileSetSize];
  curLoop->coloring = new int[toTileSetSize];
  memcpy (curLoop->tiling, loopIter2tc->iter2tile, sizeof(int)*toTileSetSize);
  memcpy (curLoop->coloring, loopIter2tc->iter2color, sizeof(int)*toTileSetSize);
#endif

  return loopIter2tc;
}

iter2tc_t* tile_backward (loop_t* curLoop,
                          projection_t* prevLoopProj)
{
  // aliases
  set_t* toTile = curLoop->set;
  int toTileSetSize = toTile->size;
  std::string toTileSetName = toTile->name;
  desc_list* descriptors = curLoop->descriptors;
  iter2tc_t *loopIter2tc;

  // the following contains all projected iteration sets that have already been
  // used to determine a tiling and a coloring for curLoop
  std::set<set_t*, bool(*)(const set_t* a, const set_t* b)> checkedSets (&set_cmp);

  // allocate and initialize space to keep tiling and coloring results
  int* loopIter2tile = new int[toTileSetSize];
  int* loopIter2color = new int[toTileSetSize];
  std::fill_n (loopIter2tile, toTileSetSize, INT_MAX);
  std::fill_n (loopIter2color, toTileSetSize, INT_MAX);
  loopIter2tc = iter2tc_init (toTileSetName, toTileSetSize, loopIter2tile, loopIter2color);

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
    iter2tc_t projIter2tc = {touchedSetName};
    projection_t::iterator iprojIter2tc = prevLoopProj->find (&projIter2tc);
    if (iprojIter2tc == prevLoopProj->end()) {
      continue;
    }
    int* projIter2tile = (*iprojIter2tc)->iter2tile;
    int* projIter2color = (*iprojIter2tc)->iter2color;

    if (touchedSet == toTile) {
      // direct set case
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

      int ariety = mapSize / toTileSetSize;

      // iterate over the iteration set of the loop we are tiling, and use the map
      // to access the indirectly touched elements
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = loopIter2tile[i];
        int iterColor = loopIter2color[i];
        for (int j = 0; j < ariety; j++) {
          int indIter = indMap[i*ariety + j];
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

    checkedSets.insert (touchedSet);
  }

#ifdef SLOPE_VTK
  // if requested at compile time, the coloring and tiling of the parloop are
  // explicitly tracked. These can be used for debugging or visualization purposes,
  // for example for generating VTK files showing the colored parloop
  curLoop->tiling = new int[toTileSetSize];
  curLoop->coloring = new int[toTileSetSize];
  memcpy (curLoop->tiling, loopIter2tc->iter2tile, sizeof(int)*toTileSetSize);
  memcpy (curLoop->coloring, loopIter2tc->iter2color, sizeof(int)*toTileSetSize);
#endif

  return loopIter2tc;
}

iter2tc_t* iter2tc_init (std::string name, int itSetSize, int* iter2tile, int* iter2color)
{
  iter2tc_t* iter2tc = new iter2tc_t;

  iter2tc->name = name;
  iter2tc->itSetSize = itSetSize;
  iter2tc->iter2tile = iter2tile;
  iter2tc->iter2color = iter2color;

  return iter2tc;
}

iter2tc_t* iter2tc_cpy (iter2tc_t* toCopy)
{
  iter2tc_t* iter2tc = new iter2tc_t;

  iter2tc->name = toCopy->name;
  iter2tc->itSetSize = toCopy->itSetSize;
  iter2tc->iter2tile = new int[toCopy->itSetSize];
  iter2tc->iter2color = new int[toCopy->itSetSize];

  memcpy (iter2tc->iter2tile, toCopy->iter2tile, sizeof(int)*toCopy->itSetSize);
  memcpy (iter2tc->iter2color, toCopy->iter2color, sizeof(int)*toCopy->itSetSize);

  return iter2tc;
}

void iter2tc_free (iter2tc_t* iter2tc)
{
  if (! iter2tc) {
    return;
  }
  delete[] iter2tc->iter2tile;
  delete[] iter2tc->iter2color;
  delete iter2tc;
}

void projection_free (projection_t* projection)
{
  projection_t::iterator it, end;
  for (it = projection->begin(), end = projection->end(); it != end; it++) {
    iter2tc_free(*it);
  }
  delete projection;
}

/***** Static / utility functions *****/

inline static void updateTilesTracker (tracker_t& iterTilesPerColor,
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
