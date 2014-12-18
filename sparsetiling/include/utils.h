/*
 *  utils.h
 *
 * Implement some utility functions and macros
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "inspector.h"

#ifndef NDEBUG
/*
 * This is taken from:
 *   http://stackoverflow.com/questions/3767869/adding-message-to-assert
 */
#   define ASSERT(condition, message) \
    do { \
      if (! (condition)) { \
          std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                    << " line " << __LINE__ << ": " << message << std::endl; \
          std::exit(EXIT_FAILURE); \
      } \
    } while (false)

#   define PRINT_INTARR(array, start, finish) \
    do { \
      std::cout << "Array `" #array "`: "; \
      for (int ia = start; ia < finish; ++ia) { \
        std::cout << array[ia] << " "; \
      } \
      std::cout << std::endl; \
    } while (false)

#   define PRINT_PROJECTION(_projection, _loop) \
    do { \
      std::cout << "Projection `" #_projection "` for loop " \
                << _loop << ":" << std::endl; \
      std::cout << "  Sets touched in previous tiled loop:" << std::endl; \
      projection_t::const_iterator it, end; \
      for (it = _projection->begin(), end = _projection->end(); it != end; it++) { \
        std::cout << "    " << (*it)->setName \
                  << ", size: " << (*it)->itSetSize << std::endl; \
      } \
    } while (false)

#else
#   define ASSERT(condition, message) do { } while (false)
#endif

/*
 * Timing function
 */
inline double time_stamp()
{
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return tv.tv_sec + (tv.tv_nsec) / (1000.0*1000.0*1000.0);
}

/*
 * Generic utility functions
 */
#define MAX(A, B) (A > B) ? A : B
#define MIN(A, B) (A < B) ? A : B

/*
 * Generate a VTK file showing the coloring of each tiled parloop once opened
 * with a program such as Paraview.
 *
 * As an example, a generated VTK file for a parloop over edges looks as follows:
 *
 * # vtk DataFile Version 1.0
 * Coloring edges
 * ASCII
 *
 * DATASET POLYDATA
 * POINTS 3 int
 * 0.0 0.0 0.0
 * 1.0 0.0 0.0
 * 0.0 1.0 0.0
 *
 * LINES 3 9
 * 2 0 1
 * 2 0 2
 * 2 2 1
 *
 * CELL_DATA 3
 * SCALARS pressure int
 * LOOKUP_TABLE default
 * 0.0
 * 0.0
 * 1.0
 *
 * on the other hand, a generated VTK file for a parloop over cells looks as follows:
 *
 * # vtk DataFile Version 1.0
 * Coloring cells
 * ASCII
 *
 * DATASET POLYDATA
 * POINTS 4 float
 * 0.0 0.0 0.0
 * 1.0 0.0 0.0
 * 0.0 1.0 0.0
 * 1.0 1.0 0.0
 *
 * POLYGONS 2 8
 * 3 0 1 2
 * 3 1 2 3
 *
 * CELL_DATA 2
 * SCALARS pressure float
 * LOOKUP_TABLE default
 * 0.0
 * 1.0
 */
#ifndef VTK_DIR
#define VTK_DIR "vtkfiles"
#endif
inline void generate_vtk (inspector_t* insp, set_t* nodes, int* coordinates, int meshDim)
{
#ifndef VTKON
  std::cout << "To enable generation of VTK files, compile with -DVTKON" << std::end;
  return;
#endif

  // aliases
  loop_list* loops = insp->loops;
  int nNodes = nodes->size;
  char* nodesSetName = nodes->setName;

  // create directory in which VTK files will be stored, if not exists
  struct stat st = {0};
  if (stat(VTK_DIR, &st) == -1)
    mkdir(VTK_DIR, 0700);

  loop_list::const_iterator it, end;
  int i = 0;
  for (it = loops->begin(), end = loops->end(); it != end; it++, i++) {
    loop_t* loop = *it;
    int loopSetSize = loop->set->size;
    char* loopName = loop->loopName;
    desc_list* descriptors = loop->descriptors;

    if (! loop->coloring) {
      std::cout << "No coloring for loop `" << loopName
                << "`, check output of inspector." << std::endl;
    }

    std::stringstream stream;
    stream << VTK_DIR << "/loop" << i << "-" << loopName << ".vtk" << std::endl;
    std::ofstream vtkfile;
    vtkfile.open (stream.str());
    stream.str("");
    stream.clear();

    // write header of the VTK file
    vtkfile << "# vtk DataFile Version 1.0" << std::endl;
    vtkfile << "Coloring %s" << loopName << std::endl;
    vtkfile << "ASCII" << std::endl << std::endl;

    // write coordinates of nodes
    vtkfile << "DATASET POLYDATA" << std::endl;
    vtkfile << "POINTS" << nNodes << " float" << std::endl;
    for (int j = 0; j < nNodes; j++) {
      vtkfile << coordinates[j*meshDim] << " " << coordinates[j*meshDim + 1];
      switch (meshDim) {
        case 2:
          vtkfile << " 0.0" << std::endl;
          break;
        case 3:
          vtkfile << " " << coordinates[j*meshDim + 2] << std::endl;
          break;
        default:
          std::cout << "Invalid mesh dimension (supported: 2, 3)";
          vtkfile.close();
          return;
      }
    }

    vtkfile << std::endl;

    // write iteration set map to nodes; if not available in the loop's descriptors,
    // just skip the file generation for the loop
    desc_list::const_iterator descIt, descEnd;
    bool found = false;
    for (descIt = descriptors->begin(), descEnd = descriptors->end(); it != end; it++) {
      map_t* map = (*descIt)->map;
      if (! strcmp (map->outSet->setName, nodesSetName)) {
        int ariety = map->mapSize;
        std::string shape = (ariety == 2) ? "LINES " : "POLYGONS ";
        stream << shape << loopSetSize << " " << loopSetSize*(ariety + 1) << std::endl;
        vtkfile << stream.str();
        for (int k = 0; k < loopSetSize; k++) {
          vtkfile << ariety;
          for (int l = 0; l < ariety; l++) {
            vtkfile << " " << map->indMap[k*ariety + l];
          }
        }
        found = true;
        break;
      }
    }
    if (! found) {
      std::cout << "Couldn't find a map to nodes for loop " << loopName << std::endl;
      vtkfile.close();
      continue;
    }

    // write colors of the iteration set elements
    vtkfile << std::endl << "CELL_DATA " << loopSetSize << std::endl;
    vtkfile << "SCALARS colors int" << std::endl;
    vtkfile << "LOOKUP_TABLE default" << std::endl;
    for (int k = 0; k < loopSetSize; k++) {
      vtkfile << loop->coloring[k] << std::endl;
    }

    vtkfile.close();
  }

}

#endif
