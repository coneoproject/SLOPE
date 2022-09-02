/*
 *  inspector.h
 *
 * Define data structure and prototypes to build an inspector
 */

#ifndef _INSPECTOR_H_
#define _INSPECTOR_H_

#include "parloop.h"
#include "tile.h"

enum insp_strategy {SEQUENTIAL, OMP, ONLY_MPI, OMP_MPI};
enum insp_coloring {COL_DEFAULT, COL_RAND, COL_MINCOLS};
enum insp_info {INSP_OK, INSP_ERR};
enum insp_verbose {MINIMAL = 1, VERY_LOW = 5, LOW = 20, MEDIUM = 40, HIGH};

/*
 * The inspector main data structure.
 */
typedef struct {
  /* unique name identifying the inspector */
  std::string name;
  /* tiling strategy: can be for sequential, openmp, or mpi execution */
  insp_strategy strategy;
  /* the seed loop index */
  int seed;
  /* seed loop partitioning */
  map_t* iter2tile;
  /* seed loop coloring */
  map_t* iter2color;
  /* average tile size */
  int avgTileSize;
  /* list of loops spanned by a tile */
  loop_list* loops;
  /* list of tiles */
  tile_list* tiles;
  /* track local, exec_halo, and non_exec_halo tiles */
  set_t* tileRegions;
  /* number of tiling sweeps */
  int nSweeps;
  /* partitioning mode */
  std::string partitioningMode;

  /* how tiles are going to be colored */
  insp_coloring coloring;
  /* the mesh structure, as a list of maps to nodes */
  map_list* meshMaps;
  /* available set partitionings, may be used for deriving tiles */
  map_list* partitionings;

  /* number of extra iterations in a tile, useful for SW prefetching */
  int prefetchHalo;

  /* should we ignore write-after-read dependencies during inspection ? */
  bool ignoreWAR;

  /* the following fields track the time spent in various code sections*/
  double totalInspectionTime;
  double partitioningTime;

  /* additional global information */
  int nThreads;

} inspector_t;

/*
 * Initialize a new inspector
 *
 * @param tileSize
 *   average tile size to be used when partitioning the seed loop
 * @param strategy
 *   tiling strategy (SEQUENTIAL; OMP - openmp; ONLY_MPI - one MPI process for
 *   each core; OMP_MPI - mixed OMP (within a node) and MPI (cross-node))
 * @param coloring
 *   by default, tiles are colored based on the inspection strategy. For example,
 *   the OMP strategy requires adjacent tiles be given different colors to preserve
 *   correctness of shared memory parallelism. However, the default behaviour of
 *   some strategies can be altered, particularly that of SEQUENTIAL and ONLY_MPI.
 *   Accepted values are COL_DEFAULT, COL_RAND (random coloring), COL_MINCOLS
 *   (try to minimize the number of colors such that adjacent tiles have different
 *   colors)
 * @param meshMaps (optional)
 *   a high level description of the mesh through a list of maps to nodes. This
 *   can optionally be used to partition an iteration space using an external
 *   library, such that tiles of particular shape (e.g., squarish, rather than
 *   stripy) can be created
 * @param partitionings (optional)
 *   one or more partitionings of sets into disjoint, contiguous subsets.
 *   Can be used in place of the default tile partitioning scheme if the seed loop
 *   iteration set is found in this list
 * @param prefetchHalo (optional)
 *   number of extra iterations in a tile, for each loop. This can simplify
 *   the implementation of software prefetching in the executor
 * @param name (optional)
 *   a unique name that identifies the inspector. Only useful if more than
 *   one inspectors are needed
 * @return
 *   an empty inspector
 */
inspector_t* insp_init (int tileSize,
                        insp_strategy strategy,
                        insp_coloring coloring = COL_DEFAULT,
                        map_list* meshMaps = NULL,
                        map_list* partitionings = NULL,
                        int prefetchHalo = 1,
                        bool ignoreWAR = false,
                        std::string name = "");

/*
 * Add a parloop to the inspector
 *
 * @param insp
 *   the inspector data structure
 * @param name
 *   a string to identify the parloop
 * @param set
 *   iteration set of the parloop
 * @param descriptors
 *   list of access descriptors used by the parloop. Each descriptor specifies
 *   what and how a set is accessed.
 * @return
 *   the inspector is added a new parloop
 */
insp_info insp_add_parloop (inspector_t* insp,
                            std::string name,
                            set_t* set,
                            desc_list* descriptors,
                            int nhalos = 1);

/*
 * Inspect a sequence of parloops and compute a tiling scheme
 *
 * @param insp
 *   the inspector data structure, already initialized with some parloops
 * @param suggestedSeed
 *   the ID of a loop from which tiling should be derived (i.e., a number between
 *   0 and the number of parloops in the inspector). This parameters may be
 *   ignored depending on the tiling strategy
 * @return
 *   populates the tiles in the inspector
 */
insp_info insp_run (inspector_t* insp,
                    int suggestedSeed);

/*
 * Print a summary of the inspector
 *
 * @param insp
 *   the inspector data structure
 * @param level
 *   level of verbosity (MINIMAL, VERY_LOW, LOW, MEDIUM, HIGH)
 * @param loopIndex
 *   if different than -1, print information only for loop of index loopIndex
 */
void insp_print (inspector_t* insp,
                 insp_verbose level,
                 int loopIndex = -1);

/*
 * Destroy an inspector
 */
void insp_free (inspector_t* insp);

#endif
