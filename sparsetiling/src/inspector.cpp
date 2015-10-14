/*
 *  inspector.cpp
 *
 * Implement inspector routines
 */

#include <string>

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "inspector.h"
#include "utils.h"
#include "partitioner.h"
#include "coloring.h"
#include "tiling.h"

using namespace std;


// prototypes of static functions
static int select_seed_loop (insp_strategy strategy, loop_list* loops, int suggestedSeed);
static void print_tiled_loop (tile_list* tiles, loop_t* loop, int verbosityTiles);


inspector_t* insp_init (int avgTileSize, insp_strategy strategy, map_list* meshMaps)
{
  inspector_t* insp = new inspector_t;

  insp->strategy = strategy;
  insp->avgTileSize = avgTileSize;
  insp->loops = new loop_list;

  insp->seed = -1;
  insp->iter2tile = NULL;
  insp->iter2color = NULL;
  insp->tiles = NULL;
  insp->nSweeps = 0;

  insp->meshMaps = meshMaps;

  return insp;
}

insp_info insp_add_parloop (inspector_t* insp, string name, set_t* set,
                            desc_list* descriptors)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  loop_t* loop = new loop_t;
  loop->name = name;
  loop->set = set;
  loop->index = insp->loops->size();
  loop->descriptors = descriptors;
  loop->coloring = NULL;
  loop->tiling = NULL;
  loop->seedMap = NULL;

  insp->loops->push_back(loop);

  return INSP_OK;
}

insp_info insp_run (inspector_t* insp, int suggestedSeed)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  // aliases
  insp_strategy strategy = insp->strategy;
  loop_list* loops = insp->loops;
  int nLoops = loops->size();

  // establish the seed loop
  int seed = select_seed_loop (strategy, loops, suggestedSeed);
  ASSERT((seed >= 0) && (seed < nLoops), "Could not find a valid seed loop");
  insp->seed = seed;

  // aliases
  loop_t* seedLoop = loops->at(seed);
  ASSERT(!seedLoop->set->superset || nLoops == 1, "Seed loop cannot be a subset");
  string seedLoopSetName = seedLoop->set->name;
  int seedLoopSetSize = seedLoop->set->size;

  // partition the seed loop iteration set into tiles
  partition (insp);
  map_t* iter2tile = insp->iter2tile;
  tile_list* tiles = insp->tiles;

  // /crossSweepConflictsTracker/ tracks color conflicts due to tiling for shared
  // memory parallelism. The data structure is empty before the first tiling attempt.
  // After each tiling sweep, it tracks, for each tile /i/, the tiles that, if assigned
  // the same color as /i/, would end up "touching" /i/ (i.e., the "conflicting" tiles),
  // leading to potential race conditions during shared memory parallel execution
  tracker_t crossSweepConflictsTracker;
  bool conflicts;
  do {
    // assume there are no color conflicts
    conflicts = false;

    // color the seed loop iteration set
    map_t* iter2color;
    if (nLoops == 1 && loop_is_direct(seedLoop)) {
      iter2color = color_fully_parallel (insp);
    }
    else if (strategy == SEQUENTIAL || strategy == ONLY_MPI) {
      iter2color = color_sequential (insp);
    }
    else if (strategy == OMP || strategy == OMP_MPI) {
      iter2color = color_shm (insp, seedLoop->seedMap, &crossSweepConflictsTracker);
    }
    else {
      ASSERT(false, "Cannot compute a seed coloring");
    }
    insp->iter2color = iter2color;

#ifdef SLOPE_VTK
    // track coloring and tiling of a parloop. These can be used for debugging or
    // visualization purpose, e.g. for generating VTK files.
    seedLoop->tiling = new int[seedLoopSetSize];
    seedLoop->coloring = new int[seedLoopSetSize];
    memcpy (seedLoop->tiling, iter2tile->values, sizeof(int)*seedLoopSetSize);
    memcpy (seedLoop->coloring, iter2color->values, sizeof(int)*seedLoopSetSize);
#endif

    // create copies of seed tiling and coloring, which will be used for
    // backward tiling (forward tiling uses and modifies the original copies)
    int* tmpIter2tileMap = new int[seedLoopSetSize];
    int* tmpIter2colorMap = new int[seedLoopSetSize];
    memcpy (tmpIter2tileMap, iter2tile->values, sizeof(int)*seedLoopSetSize);
    memcpy (tmpIter2colorMap, iter2color->values, sizeof(int)*seedLoopSetSize);

    // tile the loop chain. First forward, then backward. The algorithm is as follows:
    // 1- start from the seed loop; for each loop in the forward direction
    // 2- project the data dependencies that loop /i-1/ induces to loop /i/
    // 3- tile loop /i/, using the aforementioned projection
    // 4- go back to point 2, and repeat till there are loop along the direction
    // do the same for backward tiling

    // the tracker for conflicts arising in this tiling sweep
    tracker_t conflictsTracker;

    // prepare for forward tiling
    loop_t* prevTiledLoop = seedLoop;
    projection_t* seedLoopProj = new projection_t (&iter2tc_cmp);
    projection_t* prevLoopProj = new projection_t (&iter2tc_cmp);
    iter2tc_t* seedTilingInfo = iter2tc_init (seedLoopSetName, seedLoopSetSize,
                                              tmpIter2tileMap, tmpIter2colorMap);
    iter2tc_t* prevTilingInfo = iter2tc_cpy (seedTilingInfo);

    // forward tiling
    for (int i = seed + 1; i < nLoops; i++) {
      loop_t* curLoop = loops->at(i);
      iter2tc_t* curTilingInfo;

      // compute projection from loop /i-1/ for tiling loop /i/
      project_forward (prevTiledLoop, prevTilingInfo, prevLoopProj, seedLoopProj,
                       &conflictsTracker);

      // tile loop /i/
      curTilingInfo = tile_forward (curLoop, prevLoopProj);
      tile_assign_loop (tiles, i, curTilingInfo->itSetSize, curTilingInfo->iter2tile);

      // prepare for next loop
      prevTiledLoop = curLoop;
      prevTilingInfo = curTilingInfo;
    }

    // prepare for backward tiling
    iter2tc_free (prevTilingInfo);
    projection_free (prevLoopProj);
    prevLoopProj = seedLoopProj;
    prevTiledLoop = seedLoop;
    prevTilingInfo = seedTilingInfo;

    // backward tiling
    for (int i = seed - 1; i >= 0; i--) {
      loop_t* curLoop = loops->at(i);
      iter2tc_t* curTilingInfo;

      // compute projection from loop /i+1/ for tiling loop /i/
      project_backward (prevTiledLoop, prevTilingInfo, prevLoopProj, &conflictsTracker);

      // tile loop /i/
      curTilingInfo = tile_backward (curLoop, prevLoopProj);
      tile_assign_loop (tiles, i, curTilingInfo->itSetSize, curTilingInfo->iter2tile);

      // prepare for next loop
      prevTiledLoop = curLoop;
      prevTilingInfo = curTilingInfo;
    }

    // free memory
    iter2tc_free (prevTilingInfo);
    projection_free (prevLoopProj);

    // if color conflicts are found, we need to perform another tiling sweep this
    // time starting off with a "constrained" seed coloring
    tracker_t::const_iterator it, end;
    for (it = conflictsTracker.begin(), end = conflictsTracker.end(); it != end; it++) {
      if (it->second.size() > 0) {
        // at least one conflict, so execute another tiling sweep
        conflicts = true;
      }
      // update the cross-sweep tracker, in case there will be another sweep
      crossSweepConflictsTracker[it->first].insert(it->second.begin(), it->second.end());
    }

    insp->nSweeps++;
  } while (conflicts);

  return INSP_OK;
}

void insp_print (inspector_t* insp, insp_verbose level, int loopIndex)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  // aliases
  loop_list* loops = insp->loops;
  map_t* iter2tile = insp->iter2tile;
  map_t* iter2color = insp->iter2color;
  tile_list* tiles = insp->tiles;
  int seed = insp->seed;
  int avgTileSize = insp->avgTileSize;
  int nTiles = tiles->size();
  int nLoops = loops->size();
  int itSetSize = loops->at(seed)->set->size;

  // set verbosity level
  int verbosityItSet, verbosityTiles;
  switch (level) {
    case VERY_LOW:
      verbosityItSet = 1;
      verbosityTiles = (loopIndex == -1) ? MIN(LOW / 3, avgTileSize / 2) : INT_MAX;
      break;
    case LOW:
      verbosityItSet = MIN(LOW / 4, itSetSize);
      verbosityTiles = (loopIndex == -1) ? MIN(LOW / 3, avgTileSize / 2) : INT_MAX;
      break;
    case MEDIUM:
      verbosityItSet = MIN(MEDIUM / 2, itSetSize);
      verbosityTiles = (loopIndex == -1) ? avgTileSize : INT_MAX;
      break;
    case HIGH:
      verbosityItSet = avgTileSize;
      verbosityTiles = INT_MAX;
  }

  cout << endl << ":: SLOPE inspection summary ::" << endl << endl;
  if (loops) {
    cout << "Number of loops: " << nLoops << ", seed loop: " << seed << endl;
  }
  else {
    cout << "No loops specified" << endl;
  }
  cout << "Number of tiles: " << nTiles << endl;
  cout << "Average tile size: " << avgTileSize << endl;
  if (level != VERY_LOW) {
    if (iter2tile && iter2color) {
      cout << endl << "Printing partioning of the seed loop's iteration set:" << endl;
      cout << "  Iteration  |  Tile |  Color" << endl;
      for (int i = 0; i < itSetSize / avgTileSize; i++) {
        int offset = i*avgTileSize;
        for (int j = 0; j < verbosityItSet; j++) {
          cout << "         " << offset + j
               << "   |   " << iter2tile->values[offset + j]
               << "   |   " << iter2color->values[offset + j] << endl;
        }
        string separator = (verbosityItSet != avgTileSize) ? "         ...\n" : "";
        cout << separator;
      }
      int itSetReminder = itSetSize % avgTileSize;
      int offset = itSetSize - itSetReminder;
      for (int i = 0; i < MIN(verbosityItSet, itSetReminder); i++) {
        cout << "         " << offset + i
             << "   |   " << iter2tile->values[offset + i]
             << "   |   " << iter2color->values[offset + i] << endl;
      }
    }
    else {
      cout << "No partitioning of the seed loop performed" << endl;
    }
  }

  if (level != VERY_LOW) {
    cout << endl << "Coloring summary (color:#tiles):" << endl;
    std::map<int, int> colors;
    tile_list::const_iterator it, end;
    for (it = tiles->begin(), end = tiles->end(); it != end; it++) {
      colors[(*it)->color]++;
    }
    std::map<int, int>::const_iterator mIt, mEnd;
    for (mIt = colors.begin(), mEnd = colors.end(); mIt != mEnd; mIt++) {
      cout << mIt->first << " : " << mIt->second << endl;
    }
  }

  if (tiles && loopIndex != -2) {
    cout << endl << "Tiling computed in " << insp->nSweeps << " sweeps" << endl;
    if (loopIndex == -1) {
      cout << "Printing tiles' seed loop iterations" << endl;
      print_tiled_loop (tiles, loops->at(seed), verbosityTiles);
      if (seed + 1 < nLoops) {
        cout << endl << "Printing result of forward tiling..." << endl;
        for (int i = seed + 1; i < nLoops; i++) {
          print_tiled_loop (tiles, loops->at(i), verbosityTiles);
        }
      }
      else {
        cout << endl << "No forward tiling (seed loop is loop chain's top)" << endl;
      }
      if (0 <= seed - 1) {
        cout << endl << "Printing result of backward tiling..." << endl;
        for (int i = seed - 1; i >= 0; i--) {
          print_tiled_loop (tiles, loops->at(i), verbosityTiles);
        }
      }
      else {
        cout << endl << "No backward tiling (seed loop is loop chain's bottom)" << endl;
      }
    }
    else {
      ASSERT((loopIndex >= 0) && (loopIndex < nLoops),
             "Invalid loop index while printing inspector summary");
      cout << "Printing result of tiling loop " << loops->at(loopIndex)->name << endl;
      print_tiled_loop (tiles, loops->at(loopIndex), verbosityTiles);
    }
  }
  cout << endl;
}

void insp_free (inspector_t* insp)
{
  ASSERT(insp != NULL, "Invalid NULL pointer to inspector");

  // Note that tiles are not freed because they are already freed in the
  // executor free function

  // aliases
  loop_list* loops = insp->loops;

  // delete tiled loops, access descriptors, maps, and sets
  // freed data structures are tracked so that freeing twice the same pointer
  // is avoided
  desc_list deletedDescs;
  map_list deletedMaps;
  set_list deletedSets;
  loop_list::const_iterator lIt, lEnd;
  for (lIt = loops->begin(), lEnd = loops->end(); lIt != lEnd; lIt++) {
    desc_list* descriptors = (*lIt)->descriptors;
    desc_list::const_iterator dIt, dEnd;
    for (dIt = descriptors->begin(), dEnd = descriptors->end(); dIt != dEnd; dIt++) {
      descriptor_t* desc = *dIt;
      if (deletedDescs.find(desc) == deletedDescs.end()) {
        // found an access descriptor to be freed
        // now check if its map and sets still need be freed
        map_t* map = desc->map;
        desc->map = NULL;
        if (map != DIRECT) {
          if (deletedMaps.find(map) == deletedMaps.end()) {
            // map still to be freed, so its sets might have to be freed as well
            set_t* inSet = map->inSet;
            set_t* outSet = map->outSet;
            map->inSet = NULL;
            map->outSet = NULL;
            if (deletedSets.find(inSet) == deletedSets.end()) {
              // set still to be freed
              set_free (inSet);
              deletedSets.insert (inSet);
            }
            if (deletedSets.find(outSet) == deletedSets.end()) {
              // set still to be freed
              set_free (outSet);
              deletedSets.insert (outSet);
            }
            map_free (map);
            deletedMaps.insert (map);
          }
        }
        desc_free (desc);
        deletedDescs.insert (desc);
      }
    }
    // delete loops
#ifdef SLOPE_VTK
    delete[] (*lIt)->tiling;
    delete[] (*lIt)->coloring;
#endif
    delete *lIt;
  }

  delete insp->loops;

  map_free (insp->iter2tile, true);
  map_free (insp->iter2color, true);
  delete insp;
}

/***** Static / utility functions *****/

static void print_tiled_loop (tile_list* tiles, loop_t* loop, int verbosityTiles)
{
  // aliases
  int nTiles = tiles->size();
  int totalIterationsAssigned = 0;

  cout << "  Loop " << loop->index << " - " << loop->name << endl;
  cout << "       Tile  |  Color  |  tot : {Iterations} " << endl;

  int tilesRange = MIN(nTiles, verbosityTiles);
  for (int i = 0; i < tilesRange; i++) {
    int tileSize = tiles->at(i)->iterations[loop->index]->size();
    totalIterationsAssigned += tileSize;
    string tileInfo = (tiles->at(i)->region == LOCAL) ? "      " : "(HALO)";
    int range = MIN(tileSize, verbosityTiles);
    cout << " " << tileInfo << " " << i << "   |    " << tiles->at(i)->color << "    |   "
         << tileSize << " : {";
    if (tileSize == 0) {
      cout << "No iterations}" << endl;
      continue;
    }
    cout << tiles->at(i)->iterations[loop->index]->at(0);
    for (int j = 1; j < range; j++) {
      cout << ", " << tiles->at(i)->iterations[loop->index]->at(j);
    }
    if (tileSize > verbosityTiles) {
      int lastIterID = tiles->at(i)->iterations[loop->index]->at(tileSize - 1);
      cout << "..., " << lastIterID;
    }
    cout << "}" << endl;
  }
  if (nTiles > tilesRange) {
    for (int i = tilesRange; i < nTiles; i++) {
      int tileSize = tiles->at(i)->iterations[loop->index]->size();
      totalIterationsAssigned += tileSize;
    }
    cout << "         ..." << endl;
  }
  cout << "Summary: assigned " << totalIterationsAssigned << "/"
       << loop->set->size << " iterations" << endl;
}

static int select_seed_loop (insp_strategy strategy, loop_list* loops, int suggestedSeed)
{
  // with MPI, we only allow tiling from the bottom of the loop chain.
  // In this way, we don't have to determine the /owned/ region which would be
  // required to separate the /halo/ and /size/ regions to prevent tiles from
  // growing over /halo/.

  if (strategy == ONLY_MPI) {
    ASSERT (loops->at(suggestedSeed)->set->execHalo != 0, "Invalid HALO region");
    return 0;
  }
  if (strategy == OMP_MPI) {
    ASSERT (loops->at(suggestedSeed)->set->execHalo != 0, "Invalid HALO region");
    ASSERT (loop_load_seed_map (loops->at(0), loops), "Couldn't load a map for coloring");
    return 0;
  }
  if (strategy == OMP) {
    if (loops->size() == 1 && loop_is_direct(loops->at(0))) {
      // only one loop and the loop is direct, we can fully parallelize it
      return 0;
    }
    // the strategy involves shared memory parallelism with indirect memory
    // accesses, so we need coloring through a suitable indirection map
    if (! loop_load_seed_map (loops->at(suggestedSeed))) {
      int i = 0;
      loop_list::const_iterator it, end;
      for (it = loops->begin(), end = loops->end(); it != end; it++, i++) {
        if (loop_load_seed_map (*it)) {
          return i;
        }
      }
      ASSERT(false, "Couldn't load a map for coloring");
    }
    return suggestedSeed;
  }
  if (strategy == SEQUENTIAL) {
    return suggestedSeed;
  }
}
