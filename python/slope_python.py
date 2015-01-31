# This module provides an interface to SLOPE for python-based codes that rely
# on just-in-time compilation for running computations on (unstrctured) meshes.
#
# Author: Fabio Luporini (2015)

import ctypes


### SLOPE C-types for python-C interaction ###

class Set(ctypes.Structure):
    # Not needed in this version of the Inspector
    pass


class Dat(ctypes.Structure):
    # Not needed in this version of the Inspector
    pass


class Map(ctypes.Structure):
    _fields_ = [('map', ctypes.POINTER(ctypes.c_int)),
                ('size', ctypes.c_int)]


class Loop(ctypes.Structure):
    pass


class Inspector(object):

    ### Templates for code generation ###

    struct_map = """
typedef struct {
  int* map;
  int size;
} slope_map;
"""

    set_def = 'set_t* %s = set("%s", %d);'
    map_def = 'map_t* %s = map("%s", %s, %s, maps[%d].map, maps[%d].size);'

    code = """
#include <iostream>

#include "inspector.h"

/****************************/
// Inspector's ctypes-compatible data structures and functions
%(map_decl)s
extern "C" void inspector(slope_map maps[%(n_maps)d]);
/****************************/

void inspector(slope_map maps[%(n_maps)d]) {
  // Declare sets, maps, dats
  %(set_defs)s

  %(map_defs)s

  int avgTileSize = %(tile_size)d;

  std::cout << "Hello, World!" << std::endl;
  return;
}
"""

    def __init__(self, mode, tile_size):
        self._mode = mode
        self._tile_size = tile_size
        # Track arguments types
        self._sets, self._dats, self._maps, self._loops = [], [], [], []

    def add_sets(self, sets):
        """Add ``sets`` to this Inspector

        :param sets: iterator of 2-tuple, in which the first entry is the name of
                     the set (a string), while the second entry is the size of the
                     iteration set
        """
        self._sets = sets
        return ([], [])

    def add_maps(self, maps):
        """Add ``maps`` to this Inspector

        :param maps: iterator of 4-tuple, in which the first entry is the name of
                     the map (a string); the second and third entries represent,
                     respectively, the input and the output sets of the map (strings);
                     follows the map itself, as a numpy array of integers
        """
        ctype = Map*len(maps)
        self._maps = maps
        return (ctype, ctype(*[Map(map.ctypes.data_as(ctypes.POINTER(ctypes.c_int)),
                                   map.size) for name, _, _, map in maps]))

    def generate_code(self):
        set_defs = [Inspector.set_def % (s[0], s[0], s[1]) for s in self._sets]
        map_defs = [Inspector.map_def % (m[0], m[0], m[1], m[2], i, i)
                    for i, m in enumerate(self._maps)]

        return Inspector.code % {
            'map_decl': Inspector.struct_map,
            'set_defs': "\n  ".join(set_defs),
            'map_defs': "\n  ".join(map_defs),
            'n_maps': len(self._maps),
            'tile_size': self._tile_size
        }


class SlopeError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


# Utility functions for the caller

def get_compile_opts():
    """Return a list of options that are expected to be used when compiling the
    inspector/executor code"""
    return ["-std=c++11"]


def get_lib_name():
    """Return default name of the shared object resulting from compiling SLOPE"""
    return "st"
