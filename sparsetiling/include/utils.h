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

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

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
        std::cout << "    " << (*it)->name \
                  << ", size: " << (*it)->itSetSize << std::endl; \
      } \
    } while (false)

#   define PRINT_MAP(mapping) \
    do { \
      std::cout << "Map `" #mapping "`:" << std::endl \
                << "  name: " << mapping->name << std::endl \
                << "  size: " << mapping->mapSize << std::endl \
                << "    inSet: " << mapping->inSet->name \
                << ", size: " << mapping->inSet->size << std::endl \
                << "    outSet: " << mapping->outSet->name \
                << ", size: " << mapping->outSet->size \
                << std::endl; \
    } while (false)

#   define PRINT_VAR(var) \
    do { \
      std::cout << "`" #var "`:" << var << std::endl; \
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
#ifdef __MACH__
  /*
   * OS X does not have clock_gettime, use clock_get_time
   * This is taken from:
   *   http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
   */
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  tv.tv_sec = mts.tv_sec;
  tv.tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_MONOTONIC, &tv);
#endif
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
#define VTK_MESH2D 2
#define VTK_MESH3D 3
inline void generate_vtk (inspector_t* insp, set_t* nodes, double* coordinates,
                          int meshDim)
{
#ifndef VTKON
  std::cout << "To enable generation of VTK files, compile with -DVTKON" << std::endl;
  return;
#endif

  // aliases
  loop_list* loops = insp->loops;
  int nNodes = nodes->size;
  std::string nodesSetName = nodes->name;

  // create directory in which VTK files will be stored, if not exists
  struct stat st = {0};
  if (stat(VTK_DIR, &st) == -1)
    mkdir(VTK_DIR, 0700);

  loop_list::const_iterator it, end;
  int i = 0;
  for (it = loops->begin(), end = loops->end(); it != end; it++, i++) {
    loop_t* loop = *it;
    int loopSetSize = loop->set->size;
    std::string loopName = loop->name;
    desc_list* descriptors = loop->descriptors;

    if (! loop->coloring) {
      std::cout << "No coloring for loop `" << loopName
                << "`, check output of inspector." << std::endl;
      continue;
    }
    std::cout << "Generating VTK file for loop `" << loopName << "`..." << std::flush;

    std::stringstream stream;
    stream << VTK_DIR << "/loop" << i << "-" << loopName << ".vtk";
    std::ofstream vtkfile;
    vtkfile.open (stream.str());
    stream.str("");
    stream.clear();

    // write header of the VTK file
    vtkfile << "# vtk DataFile Version 1.0" << std::endl;
    vtkfile << "Coloring " << loopName << std::endl;
    vtkfile << "ASCII" << std::endl << std::endl;

    // write coordinates of nodes
    vtkfile << "DATASET POLYDATA" << std::endl;
    vtkfile << "POINTS " << nNodes << " float" << std::endl;
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
    desc_list::const_iterator dIt, dEnd;
    bool found = false;
    for (dIt = descriptors->begin(), dEnd = descriptors->end(); dIt != dEnd; dIt++) {
      map_t* map = (*dIt)->map;
      if (map != DIRECT && map->outSet->name == nodesSetName) {
        int ariety = map->mapSize / loopSetSize;
        std::string shape = (ariety == 2) ? "LINES " : "POLYGONS ";
        stream << shape << loopSetSize << " " << loopSetSize*(ariety + 1) << std::endl;
        vtkfile << stream.str();
        for (int k = 0; k < loopSetSize; k++) {
          vtkfile << ariety;
          for (int l = 0; l < ariety; l++) {
            vtkfile << " " << map->indMap[k*ariety + l];
          }
          vtkfile << std::endl;
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

    std::cout << "Done!" << std::endl;
    vtkfile.close();
  }

}

#endif
