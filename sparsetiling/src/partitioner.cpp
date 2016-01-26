/*
 *  partitioner.cpp
 *
 * Implement routines for partitioning iteration sets
 */

#include <omp.h>
#include "metis.h"

#include "partitioner.h"
#include "utils.h"

#include <algorithm>
#include <iostream>

static int* chunk(loop_t* seedLoop, int tileSize,
                  int* nCore, int* nExec, int* nNonExec);
static int* metis(loop_t* seedLoop, int tileSize, map_list* meshMaps,
                  int* nCore, int* nExec, int* nNonExec);

void partition (inspector_t* insp)
{
  // aliases
  insp_strategy strategy = insp->strategy;
  map_list* meshMaps = insp->meshMaps;
  int tileSize = insp->avgTileSize;
  int nLoops = insp->loops->size();
  int seed = insp->seed;
  loop_t* seedLoop = insp->loops->at(seed);
  set_t* seedLoopSet = seedLoop->set;

  // partition the seed loop iteration space (try metis first, otherwise just split
  // it into chunks)
  int *indMap = NULL;
  int nCore, nExec, nNonExec;
  if (meshMaps) {
    indMap = metis (seedLoop, tileSize, meshMaps, &nCore, &nExec, &nNonExec);
  }
  if (! indMap) {
    indMap = chunk (seedLoop, tileSize, &nCore, &nExec, &nNonExec);
  }

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
  map_t* iter2tile = map ("i2t", set_cpy(seedLoopSet), set_cpy(tileRegions),
                          indMap, seedLoopSet->size);
  tile_assign_loop (tiles, seedLoop, indMap);

  insp->tileRegions = tileRegions;
  insp->iter2tile = iter2tile;
  insp->tiles = tiles;
}

/*
 * Chunk-partition halo regions
 */
static void chunk_halo(loop_t* seedLoop, int tileSize, int tileID, int* indMap,
                       int* nExec, int* nNonExec)
{
  int setCore = seedLoop->set->core;
  int setExecHalo = seedLoop->set->execHalo;
  int setNonExecHalo = seedLoop->set->nonExecHalo;
  int setSize = seedLoop->set->size;

  // partition the exec halo region
  // this region is expected to be much smaller than core, so we first shrunk
  // /tileSize/ in order to have enough parallelism if openmp is used
  int nThreads = omp_get_max_threads();
  if (nThreads > 1) {
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
    for (; i < setExecHalo + setNonExecHalo; i++) {
      indMap[setCore + i] = tileID;
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
static int* metis(loop_t* seedLoop, int tileSize, map_list* meshMaps,
                  int* nCore, int* nExec, int* nNonExec)
{
  int i;
  int setCore = seedLoop->set->core;
  int setSize = seedLoop->set->size;

  // use the mesh description to find a suitable map for partitioning through METIS
  map_t* map = NULL;
  map_list::const_iterator it, end;
  for (it = meshMaps->begin(), end = meshMaps->end(); it != end; it++) {
    if (set_eq(seedLoop->set, (*it)->inSet) || set_eq(seedLoop->set, (*it)->outSet)) {
      map = *it;
      break;
    }
  }
  if (! map) {
    // unfortunate scenario: the user provided a mesh description, but the loop picked
    // as seed has an iteration space which is not part of the mesh description.
    // will have to revert to chunk partitioning
    return NULL;
  }

  // now partition through METIS:
  // ... mesh geometry
  int nElements = map->inSet->size;
  int nNodes = map->outSet->size;
  int nParts = nElements / tileSize;
  int arity = map->size / nElements;
  // ... data needed for partitioning
  int* indMap = new int[nElements];
  int* indNodesMap = new int[nNodes];
  int* adjncy = map->values;
  int* offsets = new int[nElements+1]();
  for (i = 1; i < nElements+1; i++) {
    offsets[i] = offsets[i-1] + arity;
  }
  // ... options
  int result, objval, ncon = 1;
  int options[METIS_NOPTIONS];
  METIS_SetDefaultOptions(options);
  options[METIS_OPTION_NUMBERING] = 0;
  options[METIS_OPTION_CONTIG] = 1;
  // ... do partition!
  result = (arity == 2) ?
    METIS_PartGraphKway (&nNodes, &ncon, offsets, adjncy, NULL, NULL, NULL,
                         &nParts, NULL, NULL, options, &objval, indMap) :
    METIS_PartMeshNodal (&nElements, &nNodes, offsets, adjncy, NULL, NULL,
                         &nParts, NULL, options, &objval, indMap, indNodesMap);
  ASSERT(result == METIS_OK, "Invalid METIS partitioning");

  // what's the target iteration set ?
  if (set_eq(seedLoop->set, map->inSet)) {
    delete[] indNodesMap;
  }
  else {
    // note: must be set_eq(seedLoop->set, map-outSet)
    delete[] indMap;
    indMap = indNodesMap;
  }
  delete[] offsets;

  // restrict partitions to the core region
  std::fill (indMap + setCore, indMap + setSize, 0);
  std::set<int> partitions (indMap, indMap + setCore);
  // ensure the set of partitions IDs is compact (i.e., if we have a partitioning
  // 0: {0,1,...}, 1: {4,5,...}, 2: {}, 3: {6,10,...} ...
  // we instead want to have
  // 0: {0,1,...}, 1: {4,5,...}, 2: {6,10,...}, ...
  std::map<int, int> mapper;
  std::set<int>::const_iterator sIt, sEnd;
  for (i = 0, sIt = partitions.begin(), sEnd = partitions.end(); sIt != sEnd; sIt++, i++) {
    mapper[*sIt] = i;
  }
  for (i = 0; i < setCore; i++) {
    indMap[i] = mapper[indMap[i]];
  }
  *nCore = partitions.size();

  // partition the exec halo region
  chunk_halo (seedLoop, tileSize, *nCore - 1, indMap, nExec, nNonExec);

  return indMap;
}
