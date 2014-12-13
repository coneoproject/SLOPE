/*
 *  utils.h
 *
 * Implement some utility functions and macros
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <iostream>

/*
 * This is taken from:
 *   http://stackoverflow.com/questions/3767869/adding-message-to-assert
 */
#ifndef NDEBUG
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

#endif
