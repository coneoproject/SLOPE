/*
 *  utils.h
 *
 * Utility functions
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <string.h>
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
        std::cout << "    " << (*it)->name  << std::endl \
                  << "      size: " << (*it)->itSetSize << std::endl \
                  << "      tiling: "; \
        PRINT_INTARR((*it)->iter2tile, 0, (*it)->itSetSize); \
        std::cout << "      coloring: "; \
        PRINT_INTARR((*it)->iter2color, 0, (*it)->itSetSize); \
      } \
    } while (false)

#   define PRINT_MAP(mapping) \
    do { \
      std::cout << "Map `" #mapping "`:" << std::endl \
                << "  name: " << mapping->name << std::endl \
                << "  size: " << mapping->size << std::endl \
                << "    inSet: " << mapping->inSet->name \
                << ", size: " << mapping->inSet->size << std::endl \
                << "    outSet: " << mapping->outSet->name \
                << ", size: " << mapping->outSet->size \
                << std::endl; \
    } while (false)

#   define PRINT_TRACKER(tracker) \
    do { \
      std::cout << "Tracker `" #tracker "`:" << std::endl; \
      int iTrack = 0; \
      for (iTrack = 0; iTrack < tracker.size(); iTrack++) { \
        std::cout << "  " << iTrack << ": "; \
        index_set tracked = tracker[iTrack]; \
        index_set::const_iterator isIt, isEnd; \
        for (isIt = tracked.begin(), isEnd = tracked.end(); isIt != isEnd; isIt++) { \
          std::cout << *isIt << ", "; \
        } \
        std::cout << std::endl; \
      } \
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
#define VTK_DIR "/home/fl1612/Projects/Firedrake/firedrake/demos/tiling/vtkfiles"
#endif

enum dimension {DIM1 = 1, DIM2 = 2, DIM3 = 3};

void generate_vtk (inspector_t* insp,
                   insp_verbose level,
                   set_t* nodes,
                   double* coordinates,
                   dimension meshDim,
                   int rank = 0);

#endif
