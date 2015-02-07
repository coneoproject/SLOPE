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

    _globaldata = {}

    ### Templates for code generation ###

    set_def = 'set_t* %s = set("%s", %d);'
    map_def = 'map_t* %s = map("%s", %s, %s, maps[%d].map, maps[%d].size);'
    desc_def = 'desc(%s, %s)'
    desc_list_def = 'desc_list %s ({%s});'
    loop_def = 'insp_add_parloop(insp, "%s", %s, &%s);'
    output_vtk = 'generate_vtk(insp, %s, (double*)coords_dat[0].data, %s);'

    code = """
#include "inspector.h"
#include "executor.h"
#include "utils.h"

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

extern "C" void* inspector(slope_map maps[%(n_maps)d],
                           slope_dat coords_dat[1]);
/****************************/

void* inspector(slope_map maps[%(n_maps)d],
                slope_dat coords_dat[1])
{
  // Declare sets, maps, dats
  %(set_defs)s

  %(map_defs)s

  %(desc_defs)s

  int avgTileSize = %(tile_size)d;
  inspector_t* insp = insp_init (avgTileSize, %(mode)s);

  %(loop_defs)s

  int seedTilePoint = %(seed)d;
  insp_run (insp, seedTilePoint);

  %(output_vtk)s

  executor_t* exec = exec_init (insp);
  insp_free (insp);
  return exec;
}
"""

    def __init__(self, mode, tile_size):
        self._mode = mode
        self._tile_size = tile_size
        self._sets, self._dats, self._maps, self._loops = [], [], [], []

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

    def set_external_dats(self):
        """Inspection/Execution can benefit of certain data fields that are not
        strictly included in the loop chain definition. For example, for debugging
        and visualization purposes, one can provide SLOPE with a coordinates field,
        which can be used to generate VTK files displaying the colored and tiled
        domain. Extra dats can be globally set using the functions ``set_...``
        accessible in this python module.

        The caller, which is constructing this Inspector, should call this method
        right before ``generate_code``, and use the returned ``argtypes`` and
        ``argvalues`` when calling the JIT-compiled inspector module, since the
        generated code will expect some extra parameters in the main function.
        """
        if not Inspector._globaldata:
            return
        if not (self._sets and self._maps and self._loops):
            raise SlopeError("Loop chain not constructed yet")

        # Handle coordinates
        coordinates = Inspector._globaldata.get('coordinates')
        if not coordinates:
            return
        set, data, _ = coordinates
        try:
            set_size = [s[1] for s in self._sets if s[0] == set][0]
        except:
            raise SlopeError("Couldn't find set `%s` for coordinates" % set)
        ctype = Dat*1
        return [(ctype, ctype(Dat(data.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                                  set_size)))]

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

        coordinates = Inspector._globaldata.get('coordinates')
        output_vtk = ""
        if coordinates:
            set, _, arity = coordinates
            output_vtk = Inspector.output_vtk % (set, arity)

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


class Executor(object):

    _ctype = ctypes.POINTER(ctypes.c_void_p)

    ### Templates for code generation ###

    code = """
#include <iostream>

#include "executor.h"
#include "utils.h"

/****************************/
// Executor's ctypes-compatible data structures and functions

extern "C" void executor(void* exec);
/****************************/

void executor(void* _exec)
{
  executor_t* exec = (executor_t*)_exec;
  std::cout << "nColors = " << exec_num_colors (exec) << std::endl;
  return;
}
"""

    def __init__(self, inspector):
        self._inspector = inspector

    def generate_code(self):
        return Executor.code


# Error handling

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
    debug_opts = []
    if Inspector._globaldata.get('coordinates'):
        debug_opts = ['-DSLOPE_VTK']
    optimization_opts = ['-O3', '-fopenmp']
    if compiler == 'intel':
        optimization_opts.extend(['-xHost', '-inline-forceinline', '-ipo'])
    return functional_opts + debug_opts + optimization_opts


def get_lib_name():
    """Return default name of the shared object resulting from compiling SLOPE"""
    return "st"


# Functions for setting global information for inspection

def set_debug_mode(coordinates):
    """Add a coordinates field such that inspection can generate VTK files
    useful for debugging and visualization purposes.

    :param coordinates: a 3-tuple, in which the first entry is the set name the
                        coordinates belong to; the second entry is a numpy array of
                        coordinates values; the third entry indicates the dimension
                        of the dataset (accepted [1, 2, 3], for 1D, 2D, and 3D
                        datasets, respectively)
    """
    set, data, arity = coordinates
    if arity not in [1, 2, 3]:
        raise SlopeError("Arity should be a number in [1, 2, 3]")
    arity = "VTK_MESH%dD" % arity
    Inspector._globaldata['coordinates'] = (set, data, arity)
