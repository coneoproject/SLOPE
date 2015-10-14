/*
 *  partitioner.cpp
 *
 * Implement routines for partitioning iteration sets
 */

#include <omp.h>
#include "metis.h"

#include "partitioner.h"
#include "utils.h"

static int* chunk(loop_t* seedLoop, int tileSize, int* nCore, int* nExec, int* nNonExec);
static int* metis(loop_t* seedLoop, int tileSize, map_list* meshMaps, int* nCore, int* nExec, int* nNonExec);

void partition (inspector_t* insp)
{
  // aliases
  insp_strategy strategy = insp->strategy;
  map_list* meshMaps = insp->meshMaps;
  int tileSize = insp->avgTileSize;
  int nLoops = insp->loops->size();
  int seed = insp->seed;
  loop_t* seedLoop = insp->loops->at(seed);
  int setSize = seedLoop->set->size;

  // partition the seed loop iteration space
  int nCore, nExec, nNonExec;
  int *indMap = (meshMaps) ? metis (seedLoop, tileSize, meshMaps, &nCore, &nExec, &nNonExec) :
                             chunk (seedLoop, tileSize, &nCore, &nExec, &nNonExec);

  // initialize tiles:
  // ... start with creating as many empty tiles as needed ...
  int t;
  tile_list* tiles = new tile_list (nCore + nExec + nNonExec);
  for (t = 0; t < nCore; t++) {
    tiles->at(t) = tile_init (nLoops);
  }
  for (; t < nCore + nExec; t++) {
    tiles->at(t) = tile_init (nLoops, EXEC_HALO);
  }
  for (; t < nCore + nExec + nNonExec; t++) {
    tiles->at(t) = tile_init (nLoops, NON_EXEC_HALO);
  }
  // ... explicitly track the tile region (core, exec_halo, and non_exec_halo) ...
  set_t* tileRegions = set("tiles", nCore, nExec, nNonExec);
  // ... and, finally, map the partitioned seed loop to tiles
  map_t* iter2tile = map ("i2t", set_cpy(seedLoop->set), set_cpy(tileRegions),
                          indMap, setSize);
  tile_assign_loop (tiles, seed, setSize, indMap);

  insp->tileRegions = tileRegions;
  insp->iter2tile = iter2tile;
  insp->tiles = tiles;
}

/*
 * Chunk-partition halo regions
 */
static void chunk_halo(loop_t* seedLoop, int tileSize, int tileID, int* indMap, int* nExec, int* nNonExec)
{
  int setCore = seedLoop->set->core;
  int setExecHalo = seedLoop->set->execHalo;
  int setNonExecHalo = seedLoop->set->nonExecHalo;
  int setSize = seedLoop->set->size;

  // partition the exec halo region
  // this region is expected to be smaller than core, so we shrunk /tileSize/
  // accordingly to have enough parallelism
  int nThreads = omp_get_max_threads();
  if (setExecHalo <= nThreads) {
    tileSize = setExecHalo;
  }
  else if (setExecHalo > nThreads*2) {
    tileSize = setExecHalo / (nThreads*2);
  }
  else {
    // nThreads < setExecHalo < nThreads*2
    tileSize = setExecHalo / nThreads;
  }
  int i = 0;
  int nParts = (setExecHalo > 0) ? setExecHalo / tileSize : 0;
  int remainderTileSize = (setExecHalo > 0) ? setExecHalo % tileSize : 0;
  *nExec = nParts + ((remainderTileSize > 0) ? 1 : 0);
  for (; i < setExecHalo - remainderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[setCore + i] = tileID;
  }
  tileID = (remainderTileSize > 0) ? tileID + 1 : tileID;
  for (; i < setExecHalo; i++) {
    indMap[setCore + i] = tileID;
  }

  // partition the non-exec halo region
  // this is never going to be executed, so a single tile is fine
  *nNonExec = 0;
  if (setNonExecHalo > 0) {
    *nNonExec = 1;
    tileID++;
    for (; i < setSize; i++) {
      indMap[i] = tileID;
    }
  }
}

/*
 * Assign loop iterations to tiles sequentially as blocks of /tileSize/ elements
 */
static int* chunk(loop_t* seedLoop, int tileSize, int* nCore, int* nExec, int* nNonExec)
{
  int setCore = seedLoop->set->core;
  int setSize = seedLoop->set->size;

  // partition the local core region
  int i = 0;
  int tileID = -1;
  int* indMap = new int[setSize];
  int nParts = setCore / tileSize;
  int remainderTileSize = setCore % tileSize;
  *nCore = nParts + ((remainderTileSize > 0) ? 1 : 0);
  for (; i < setCore - remainderTileSize; i++) {
    tileID = (i % tileSize == 0) ? tileID + 1 : tileID;
    indMap[i] = tileID;
  }
  tileID = (remainderTileSize > 0) ? tileID + 1 : tileID;
  for (; i < setCore; i++) {
    indMap[i] = tileID;
  }

  // partition the exec halo region
  chunk_halo(seedLoop, tileSize, tileID, indMap, nExec, nNonExec);

  return indMap;
}

/*
 * Assign loop iterations to tiles carving partitions out of /seedLoop/ using
 * the METIS library.
 */
static int* metis(loop_t* seedLoop, int tileSize, map_list* meshMaps, int* nCore, int* nExec, int* nNonExec)
{
  int setSize = seedLoop->set->size;

  // use the mesh description to find a suitable map for partitioning through METIS
  map_t* iter2nodes = NULL;
  map_list::const_iterator it, end;
  for (it = meshMaps->begin(), end = meshMaps->end(); it != end; it++) {
    if (set_eq(seedLoop->set, (*it)->inSet)) {
      iter2nodes = *it;
      break;
    }
  }
  if (! iter2nodes) {
    // unfortunate scenario: the user provided a mesh description, but the loop picked
    // as seed has an iteration space which is not part of the mesh description.
    // Revert to chunk partitioning
    return chunk (seedLoop, tileSize, nCore, nExec, nNonExec);
  }

  // partition the local core region through METIS
  int nElements = iter2nodes->inSet->core;
  int nNodes = iter2nodes->outSet->core;
  int arity = iter2nodes->size / setSize;
  *nCore = nElements / tileSize;
  int* indMap = new int[setSize];
  int* indNodesMap = new int[nNodes];
  int* offsets = new int[nElements+1]();
  for (int i = 1; i < nElements+1; i++) {
    offsets[i] = offsets[i-1] + arity;
  }
  int* adjncy = iter2nodes->values;
  // partitioning related options
  int options[METIS_NOPTIONS];
  METIS_SetDefaultOptions(options);
  options[METIS_OPTION_NUMBERING] = 0;
  options[METIS_OPTION_CONTIG] = 1;
  int result, objval, ncon = 1;
  result = (arity == 2) ?
    METIS_PartGraphKway(&nNodes, &ncon, offsets, adjncy, NULL, NULL, NULL,
                        nCore, NULL, NULL, options, &objval, indMap) :
    METIS_PartMeshNodal(&nElements, &nNodes, offsets, adjncy, NULL, NULL,
                        nCore, NULL, options, &objval, indMap, indNodesMap);
  ASSERT(result == METIS_OK, "Invalid METIS partitioning");

  // partition the exec halo region
  chunk_halo(seedLoop, tileSize, *nCore, indMap, nExec, nNonExec);

  delete indNodesMap;
  delete offsets;
  return indMap;
}
