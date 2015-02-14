# This module provides an interface to SLOPE for python-based codes that rely
# on just-in-time compilation for running computations on (unstrctured) meshes.
#
# Author: Fabio Luporini (2015)

import ctypes


### SLOPE C-types for python-C interaction ###

class Set(ctypes.Structure):
    _fields_ = [('name', ctypes.c_char_p),
                ('size', ctypes.c_int)]


class Dat(ctypes.Structure):
    _fields_ = [('data', ctypes.POINTER(ctypes.c_double)),
                ('size', ctypes.c_int)]


class Map(ctypes.Structure):
    _fields_ = [('name', ctypes.c_char_p),
                ('map', ctypes.POINTER(ctypes.c_int)),
                ('size', ctypes.c_int)]


class Inspector(object):

    _globaldata = {}

    ### Templates for code generation ###

    set_def = 'set_t* %s = set(sets[%d].name, sets[%d].size);'
    map_def = 'map_t* %s = map(maps[%d].name, %s, %s, maps[%d].map, maps[%d].size);'
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
  char* name;
  int size;
} slope_set;

typedef struct {
  char* name;
  int* map;
  int size;
} slope_map;

typedef struct {
  void* data;
  int size;
} slope_dat;

extern "C" void* inspector(slope_set sets[%(n_sets)d],
                           slope_map maps[%(n_maps)d],
                           slope_dat coords_dat[1],
                           int tileSize);
/****************************/

void* inspector(slope_set sets[%(n_sets)d],
                slope_map maps[%(n_maps)d],
                slope_dat coords_dat[1],
                int tileSize)
{
  // Declare sets, maps, dats
  %(set_defs)s

  %(map_defs)s

  %(desc_defs)s

  int avgTileSize = tileSize;
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

    def __init__(self, mode):
        self._mode = mode
        self._sets, self._dats, self._maps, self._loops = [], [], [], []

    def add_sets(self, sets):
        """Add ``sets`` to this Inspector

        :param sets: iterator of 2-tuple, in which the first entry is the name of
                     the set (a string), while the second entry is the size of the
                     iteration set
        """
        ctype = Set*len(sets)
        self._sets = sets
        return (ctype, ctype(*[Set(name, size) for name, size in sets]))

    def add_maps(self, maps):
        """Add ``maps`` to this Inspector

        :param maps: iterator of 4-tuple, in which the first entry is the name of
                     the map (a string); the second and third entries represent,
                     respectively, the input and the output sets of the map (strings);
                     follows the map itself, as a numpy array of integers
        """
        ctype = Map*len(maps)
        self._maps = maps
        return (ctype, ctype(*[Map(name,
                                   map.ctypes.data_as(ctypes.POINTER(ctypes.c_int)),
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

    def set_tile_size(self, tile_size):
        """Set a tile size for this Inspector"""
        ctype = ctypes.c_int
        return (ctype, ctype(tile_size))

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
        return (ctype, ctype(Dat(data.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                                 set_size)))

    def generate_code(self):
        set_defs = [Inspector.set_def % (s[0], i, i) for i, s in enumerate(self._sets)]
        map_defs = [Inspector.map_def % (m[0], i, m[1], m[2], i, i)
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
            'n_sets': len(self._sets),
            'mode': self._mode,
            'seed': len(self._loops) / 2,
            'output_vtk': output_vtk
        }


class Executor(object):

    ### Templates for code generation ###

    meta = {
        'tile_start': 0,
        'tile_end': 'tileLoopSize',
        'name_exec': 'exec',
        'name_param_exec': '_exec',
        'name_local_map': 'loc_%(gmap)s_%(loop_id)d',
        'name_local_iters': 'iterations_%(loop_id)d',
        'loop_chain_body': '%(loop_chain_body)s',  # Instantiated user side
        'headers': ['#include "%s"' % h for h in ['inspector.h', 'executor.h',
                                                  'utils.h']],
        'ctype_exec': 'void*',
        'py_ctype_exec': ctypes.POINTER(ctypes.c_void_p)
    }

    init_code = """
executor_t* %(name_exec)s = (executor_t*)_%(name_exec)s;
int nColors = exec_num_colors (%(name_exec)s);
"""
    outer_tiles_loop = """\
for (int i = 0; i < nColors; i++) {
  const int nTilesPerColor = exec_tiles_per_color (%(name_exec)s, i);
  #ifdef SLOPE_OMP
  #pragma omp parallel for
  #endif
  for (int j = 0; j < nTilesPerColor; j++) {
    // execute tile j for color i
    tile_t* tile = exec_tile_at (%(name_exec)s, i, j);
    int %(tile_end)s;

    %(loop_chain_body)s
  }
}
"""
    local_map_def = """
iterations_list& %(lmap)s = tile_get_local_map (tile, %(loop_id)d, "%(gmap)s");
"""
    local_iters = """\
iterations_list& %(local_iters)s = tile_get_iterations (tile, %(loop_id)d);
tileLoopSize = iterations_%(loop_id)d.size();
"""

    def __init__(self, inspector):
        self._code = "\n".join([Executor.init_code % Executor.meta,
                                Executor.outer_tiles_loop % Executor.meta])
        self._loop_init, self._gtl_maps = self._generate_loops(inspector._loops)

    def _generate_loops(self, loops):
        """Return a 2-tuple, in which:

            * the first entry represents initialization code to be executed right
              before the tile's loop invoking the kernel;
            * the second entry is a list of dict that binds, for each loop, global
              maps to local maps, as well as local iterations to the actual tile's
              iterations.
        """
        header_code = []
        gtl_maps = []  # gtl stands for global-to-local
        for i, loop in enumerate(loops):
            descs = loop[2]
            global_maps = set(desc[0] for desc in descs if desc[0] != 'DIRECT')
            local_maps = [Executor.meta['name_local_map'] % {'gmap': gmap, 'loop_id': i}
                          for gmap in global_maps]
            gtl_map = dict(zip(global_maps, local_maps))
            local_maps_def = "".join([Executor.local_map_def % {
                'gmap': gmap,
                'lmap': lmap,
                'loop_id': i} for gmap, lmap in gtl_map.items()])
            name_local_iters = Executor.meta['name_local_iters'] % {
                'loop_id': i
            }
            local_iters = Executor.local_iters % {
                'local_iters': name_local_iters,
                'loop_id': i
            }
            header_code.append(("%s%s" % (local_maps_def, local_iters)).strip('\n'))
            gtl_map.update({'DIRECT': name_local_iters})
            gtl_maps.append(gtl_map)

        return (header_code, gtl_maps)

    @property
    def c_type_exec(self):
        return Executor.meta['type_exec']

    @property
    def c_name_exec(self):
        return "_%s" % Executor.meta['name_exec']

    @property
    def c_loop_start_name(self):
        return Executor.meta['tile_start']

    @property
    def c_loop_end_name(self):
        return Executor.meta['tile_end']

    @property
    def c_loop_init(self):
        return self._loop_init

    @property
    def c_headers(self):
        return Executor.meta['headers']

    @property
    def gtl_maps(self):
        return self._gtl_maps

    def c_code(self, loop_chain_body):
        return self._code % {'loop_chain_body': loop_chain_body}


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


def get_include_dir():
    """Return the include directory relative to the main folder"""
    return "sparsetiling/include"


def get_lib_dir():
    """Return the lib directory relative to the main folder"""
    return "lib"


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
