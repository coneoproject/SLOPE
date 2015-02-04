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
    _fields_ = [('data', ctypes.POINTER(ctypes.c_double)),
                ('size', ctypes.c_int)]


class Map(ctypes.Structure):
    _fields_ = [('map', ctypes.POINTER(ctypes.c_int)),
                ('size', ctypes.c_int)]


class Loop(ctypes.Structure):
    pass


class Inspector(object):

    ### Templates for code generation ###

    set_def = 'set_t* %s = set("%s", %d);'
    map_def = 'map_t* %s = map("%s", %s, %s, maps[%d].map, maps[%d].size);'
    desc_def = 'desc(%s, %s)'
    desc_list_def = 'desc_list %s ({%s});'
    loop_def = 'insp_add_parloop(insp, "%s", %s, &%s);'
    output_vtk = 'generate_vtk(insp, %s, coords_dat[0].data, %s)'

    code = """
#include <iostream>

#include "inspector.h"

/****************************/
// Inspector's ctypes-compatible data structures and functions
typedef struct {
  int* map;
  int size;
} slope_map;

typedef struct {
  void* data;
  int size;
} slope_dat;

extern "C" void inspector(slope_map maps[%(n_maps)d],
                          slope_dat coords_dat[1]);
/****************************/

void inspector(slope_map maps[%(n_maps)d],
               slope_dat coords_dat[1]) {
  // Declare sets, maps, dats
  %(set_defs)s

  %(map_defs)s

  %(desc_defs)s

  int avgTileSize = %(tile_size)d;
  inspector_t* insp = insp_init(avgTileSize, %(mode)s);

  %(loop_defs)s

  int seedTilePoint = %(seed)d;
  insp_run (insp, seedTilePoint);

  std::cout << "Hello, World!" << std::endl;

  %(output_vtk)s
  return;
}
"""

    def __init__(self, mode, tile_size):
        self._mode = mode
        self._tile_size = tile_size
        # Track arguments types
        self._sets, self._dats, self._maps, self._loops = [], [], [], []
        self._coords = None

    def add_coords(self, coords):
        """Add coordinates ``coords`` of some set to this Inspector

        Note: this method is meant to be called after the ``add_sets`` method,
              which is essential to tell this Inspector which sets compose the
              domain being tiled.

        :param coords: a 3-tuple, in which the first entry is the name of set the
                       coordinates belong to; the second entry is the numpy array of
                       coordinates values; the third entry indicates the dimension
                       of the dataset (accepted [1, 2, 3], for 1D, 2D, and 3D
                       datasets, respectively)
        """
        if not self._sets:
            raise SlopeError("`add_coords` should be called only after `add_sets`")

        set_name, data, arity = coords

        # Check parameters
        try:
            coords_size = [s[1] for s in self._sets if s[0] == set_name][0]
        except:
            raise SlopeError("Couldn't find set `%s` for coordinates" % set_name)
        if arity not in [1, 2, 3]:
            raise SlopeError("Arity should be a number between 1 and 3")
        arity = "VTK_MESH%dD" % arity

        ctype = Dat*1
        self._coords = (set_name, data, arity)
        return (ctype, ctype(Dat(data.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                                 coords_size)))

    def add_sets(self, sets):
        """Add ``sets`` to this Inspector

        :param sets: iterator of 2-tuple, in which the first entry is the name of
                     the set (a string), while the second entry is the size of the
                     iteration set
        """
        self._sets = sets

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

    def add_loops(self, loops):
        """Add a list of ``loops`` to this Inspector

        :param loops: iterator of 3-tuple ``(name, it_space, desc)``, where:
            * ``name`` is the identifier name of the loop
            * ``it_space`` is the iteration space of the loop
            * ``desc`` represents a list of descriptors. In SLOPE, a descriptor
                       specifies the memory access pattern in a loop. In particular,
                       a descriptor is a 2-tuple in which the first entry is a map
                       (previously defined through a call to ``map(...)``) and the
                       second entry is the access mode (e.g., RW, READ, INC, ...).
                       If the access to a dataset does not involve any map, than the
                       first entry assumes the value of the special keyword ``DIRECT``
        """
        self._loops = loops

    def generate_code(self):
        set_defs = [Inspector.set_def % (s[0], s[0], s[1]) for s in self._sets]
        map_defs = [Inspector.map_def % (m[0], m[0], m[1], m[2], i, i)
                    for i, m in enumerate(self._maps)]
        desc_defs, loop_defs = [], []
        for i, loop in enumerate(self._loops):
            loop_name, loop_it_space, loop_descs = loop

            descs_name = "%s_Desc_%d" % (loop_name, i)
            loop_name = "%s_Loop_%d" % (loop_name, i)

            descs = [Inspector.desc_def % (d[0], d[1]) for d in loop_descs]
            desc_defs.append(Inspector.desc_list_def % (descs_name, ", ".join(descs)))
            loop_defs.append(Inspector.loop_def % (loop_name, loop_it_space, descs_name))

        output_vtk = ""
        if self._coords:
            set_name, _, arity = self._coords
            output_vtk = Inspector.output_vtk % (set_name, arity)

        return Inspector.code % {
            'set_defs': "\n  ".join(set_defs),
            'map_defs': "\n  ".join(map_defs),
            'desc_defs': "\n  ".join(desc_defs),
            'loop_defs': "\n  ".join(loop_defs),
            'n_maps': len(self._maps),
            'tile_size': self._tile_size,
            'mode': self._mode,
            'seed': len(self._loops) / 2,
            'output_vtk': output_vtk
        }


class SlopeError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


# Utility functions for the caller

def get_compile_opts(compiler='gnu'):
    """Return a list of options that are expected to be used when compiling the
    inspector/executor code. Supported compilers: [gnu (default), intel]."""
    functional_opts = ['-std=c++11']
    debug_opts = ['-DSLOPE_VTK'] if self._coords else []
    optimization_opts = ['-O3', '-fopenmp']
    if compiler == 'intel':
        optimization_opts.extend(['-xHost', '-inline-forceinline', '-ipo'])
    return functional_opts + debug_opts + optimization_opts


def get_lib_name():
    """Return default name of the shared object resulting from compiling SLOPE"""
    return "st"
