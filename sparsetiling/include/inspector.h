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
enum insp_info {INSP_OK, INSP_ERR};
enum insp_verbose {MINIMAL = 1, VERY_LOW = 5, LOW = 20, MEDIUM = 40, HIGH};

/*
 * The inspector main data structure.
 */
typedef struct {
  /* tiling strategy: can be for sequential, openmp, or mpi execution */
  insp_strategy strategy;
  /* the seed loop index */
  int seed;
  /* partitioning of the base loop */
  map_t* iter2tile;
  /* coloring of the base loop */
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
  /* the mesh structure, as a list of maps to nodes */
  map_list* meshMaps;

} inspector_t;

/*
 * Initialize a new inspector
 *
 * @param tileSize
 *   average tile size after partitioning of the base loop's iteration set
 * @param strategy
 *   tiling strategy (SEQUENTIAL; OMP - openmp; ONLY_MPI - one MPI process for
 *   each core; OMP_MPI - mixed OMP (within a node) and MPI (cross-node))
 * @param meshMaps (optional)
 *   a high level description of the mesh through a list of maps to nodes. This
 *   can optionally be used to partition an iteration space using an external
 *   library, such that tiles of particular shape (e.g., squarish, rather than
 *   strip-like) can be carved.
 * @return
 *   an inspector data structure
 */
inspector_t* insp_init (int tileSize, insp_strategy strategy, map_list* meshMaps = NULL);

/*
 * Add a parloop to the inspector
 *
 * @param insp
 *   the inspector data structure
 * @param name
 *   identifier name of the parloop
 * @param set
 *   iteration set of the parloop
 * @param descriptors
 *   list of access descriptors used by the parloop. Each descriptor specifies
 *   what and how a set is accessed.
 * @return
 *   the inspector is updated with a new loop the tiles will have to cross
 */
insp_info insp_add_parloop (inspector_t* insp, std::string name, set_t* set,
                            desc_list* descriptors);

/*
 * Inspect a sequence of parloops and compute a tiled scheduling
 *
 * @param insp
 *   the inspector data structure, already defined over a range of parloops
 * @param suggestedSeed
 *   user-provided start point of the tiling (a number between 0 and the number of
 *   parloops crossed by the inspector). The loop has to be an indirect one if the
 *   inspector strategy targets shared memory parallelism
 * @return
 *   on return from the function, insp will contain a list of tiles, each tile
 *   characterized by a list of iterations that are supposed to be executed,
 *   for each crossed parloop
 */
insp_info insp_run (inspector_t* insp, int suggestedSeed);

/*
 * Print a summary of the inspector
 *
 * @param insp
 *   the inspector data structure
 * @param level
 *   level of verbosity (LOW, MEDIUM, HIGH)
 * @param loopIndex
 *   if different than -1, print information only for loop of index loopIndex
 */
void insp_print (inspector_t* insp, insp_verbose level, int loopIndex = -1);

/*
 * Destroy an inspector
 *
 * @param insp
 *   the inspector data structure
 */
void insp_free (inspector_t* insp);

#endif
