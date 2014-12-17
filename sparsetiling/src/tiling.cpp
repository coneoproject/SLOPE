/*
 *  tiling.cpp
 *
 * Collection of data structures and functions implementing the tiling engine
 */

#include "tiling.h"
#include "utils.h"

iter2tc_t* iter2tc_init (char* setName, int itSetSize, int* iter2tile, int* iter2color)
{
  iter2tc_t* iter2tc = (iter2tc_t*) malloc (sizeof(iter2tc_t));

  iter2tc->setName = setName;
  iter2tc->itSetSize = itSetSize;
  iter2tc->iter2tile = iter2tile;
  iter2tc->iter2color = iter2color;

  return iter2tc;
}

iter2tc_t* iter2tc_cpy (iter2tc_t* toCopy)
{
  iter2tc_t* iter2tc = (iter2tc_t*) malloc (sizeof(iter2tc_t));

  iter2tc->setName = toCopy->setName;
  iter2tc->itSetSize = toCopy->itSetSize;
  iter2tc->iter2tile = (int*) malloc (sizeof(int)*toCopy->itSetSize);
  iter2tc->iter2color = (int*) malloc (sizeof(int)*toCopy->itSetSize);

  memcpy (iter2tc->iter2tile, toCopy->iter2tile, sizeof(int)*toCopy->itSetSize);
  memcpy (iter2tc->iter2color, toCopy->iter2color, sizeof(int)*toCopy->itSetSize);

  return iter2tc;
}

void iter2tc_free (iter2tc_t* iter2tc)
{
  free (iter2tc->iter2tile);
  free (iter2tc->iter2color);
  free (iter2tc);
}

void projection_free (projection_t* projection)
{
  projection_t::iterator it, end;
  for (it = projection->begin(), end = projection->end(); it != end; it++) {
    iter2tc_free(*it);
  }
  delete projection;
}

void project_forward (loop_t* tiledLoop, iter2tc_t* tilingInfo,
                      projection_t* prevLoopProj)
{
  // aliases
  desc_list* descriptors = tiledLoop->descriptors;
  int* iter2tile = tilingInfo->iter2tile;
  int* iter2color = tilingInfo->iter2color;

  desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    // aliases
    map_t* descMap = (*it)->map;
    am_t descMode = (*it)->mode;

    iter2tc_t* projIter2tc;
    if (descMap == DIRECT) {
      // direct set case: just replicate the current tiling
      // note that multiple DIRECT descriptors have no effect on the projection,
      // since a requirement for a projection is that its elements are unique
      projIter2tc = tilingInfo;
    }
    else {
      // indirect set case
      // aliases
      int tiledSetSize = descMap->inSet->size;
      int projSetSize = descMap->outSet->size;
      char* projSetName = descMap->outSet->setName;
      int mapSize = descMap->mapSize;
      int* indMap = descMap->indMap;

      int ariety = mapSize / tiledSetSize;
      int* projIter2tile = (int*) malloc (sizeof(int)*projSetSize);
      int* projIter2color = (int*) malloc (sizeof(int)*projSetSize);
      std::fill_n (projIter2tile, projSetSize, -1);
      std::fill_n (projIter2color, projSetSize, -1);

      projIter2tc = iter2tc_init (projSetName, projSetSize, projIter2tile,
                                  projIter2color);

      // iterate over the tiledLoop's iteration set, and use the map to access
      // the touched iteration set's elements.
      for (int i = 0; i < tiledSetSize; i++) {
        int iterTile = iter2tile[i];
        int iterColor = iter2color[i];
        for (int j = 0; j < ariety; j++) {
          int indIter = indMap[i*ariety + j];
          int indColor = MAX(iterColor, projIter2color[indIter]);
          if (indColor != projIter2color[indIter]) {
            // update color and tile of the indirect touched iteration
            projIter2tile[indIter] = iterTile;
            projIter2color[indIter] = indColor;
          }
        }
      }
    }

    // Update the projection for the parloop by first removing any former
    // projection over a set touched by the previous parloop
    // Note that erase and insert are based on the set name, while the other
    // members are just ignored; here, therefore, we exploit the set name in
    // *projIter2tc to remove any now-old projection over the set, and we then
    // reinsert the object with the updated members.
    projection_t* curLoopProj = prevLoopProj;  // just for intuition
    projection_t::iterator toFree = curLoopProj->find (projIter2tc);
    if (toFree != curLoopProj->end()) {
      curLoopProj->erase(toFree);
      iter2tc_free (*toFree);
    }
    curLoopProj->insert (projIter2tc);
  }
}

void project_backward (loop_t* tiledLoop, iter2tc_t* tilingInfo,
                       projection_t* prevLoopProj)
{

}

iter2tc_t* tile_forward (loop_t* curLoop, projection_t* prevLoopProj)
{
  // aliases
  set_t* toTile = curLoop->set;
  int toTileSetSize = toTile->size;
  char* toTileSetName = toTile->setName;
  desc_list* descriptors = curLoop->descriptors;
  iter2tc_t *loopIter2tc;

  // the following contains all projected iteration sets that have already been
  // used to determine a tiling and a coloring for curLoop
  std::set<set_t*, bool(*)(const set_t* a, const set_t* b)> checkedSets (&set_cmp);

  // allocate and initialize space to keep tiling and coloring results
  int* loopIter2tile = (int*) malloc (sizeof(int)*toTileSetSize);
  int* loopIter2color = (int*) malloc (sizeof(int)*toTileSetSize);
  std::fill_n (loopIter2tile, toTileSetSize, -1);
  std::fill_n (loopIter2color, toTileSetSize, -1);
  loopIter2tc = iter2tc_init (toTileSetName, toTileSetSize, loopIter2tile,
                              loopIter2color);

  desc_list::const_iterator it, end;
  for (it = descriptors->begin(), end = descriptors->end(); it != end; it++) {
    // aliases
    map_t* descMap = (*it)->map;
    am_t descMode = (*it)->mode;
    set_t* touchedSet = (descMap == DIRECT) ? toTile : descMap->outSet;
    char* touchedSetName = touchedSet->setName;

    if (checkedSets.find(touchedSet) != checkedSets.end()) {
      // set already used for computing a tiling of curLoop (e.g. it was found
      // in a previous descriptor), so skip recomputing same information
      continue;
    }

    // retrieve projected iteration-set to tile-color info
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
      int mapSize = descMap->mapSize;
      int* indMap = descMap->indMap;

      int ariety = mapSize / toTileSetSize;

      // iterate over the being-tiled loop's iteration set, and use the map to
      // access the touched iteration set's elements.
      for (int i = 0; i < toTileSetSize; i++) {
        int iterTile = loopIter2tile[i];
        int iterColor = loopIter2color[i];
        for (int j = 0; j < ariety; j++) {
          int indIter = indMap[i*ariety + j];
          int indColor = MAX(iterColor, projIter2color[indIter]);
          if (indColor != loopIter2color[i]) {
            // update color and tile of the loop being tiled
            loopIter2tile[i] = iterTile;
            loopIter2color[i] = indColor;
          }
        }
      }
    }

    checkedSets.insert (touchedSet);
  }

  return loopIter2tc;
}

void tile_backward ()
{

}
