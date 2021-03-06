# This module provides an interface to SLOPE for python-based codes that rely
# on just-in-time compilation for running computations on (unstrctured) meshes.
#
# Author: Fabio Luporini (2015)

import ctypes


### SLOPE C-types for python-C interaction ###

class Set(ctypes.Structure):
    _fields_ = [('name', ctypes.c_char_p),
                ('core', ctypes.c_int),
                ('exec', ctypes.c_int),
                ('nonexec', ctypes.c_int)]


class Dat(ctypes.Structure):
    _fields_ = [('data', ctypes.POINTER(ctypes.c_double)),
                ('size', ctypes.c_int)]


class Map(ctypes.Structure):
    _fields_ = [('name', ctypes.c_char_p),
                ('map', ctypes.POINTER(ctypes.c_int)),
                ('size', ctypes.c_int)]


class Part(ctypes.Structure):
    _fields_ = [('name', ctypes.c_char_p),
                ('data', ctypes.POINTER(ctypes.c_int)),
                ('size', ctypes.c_int),
                ('nparts', ctypes.c_int)]


class Inspector(object):

    _globaldata = {
        'mode': 'SEQUENTIAL',
        'coloring': 'DEFAULT',
    }

    ### Templates for code generation ###

    set_def = 'set_t* %s = set(sets[%d].name, sets[%d].core, sets[%d].exec, sets[%d].nonexec, %s);'
    map_def = 'map_t* %s = map(maps[%d].name, %s, %s, maps[%d].map, maps[%d].size);'
    mesh_map_def = 'map_t* %s = map(mesh_maps[%d].name, %s, %s, mesh_maps[%d].map, mesh_maps[%d].size);'
    part_def = 'set_t* %s = set(partitionings[%d].name, partitionings[%d].nparts, 0, 0, NULL);\n  '
    part_def += 'map_t* %s = map(%s, %s, %s, partitionings[%d].part, partitionings[%d].size);'
    desc_def = 'desc(%s, %s)'
    desc_list_def = 'desc_list %s ({%s});'
    loop_def = 'insp_add_parloop(insp, "%s", %s, &%s);'
    output_vtk = 'generate_vtk(insp, %s, %s, (double*)coords_dat[0].data, %s, rank);'
    output_insp = 'insp_print (insp, %s);'

    code = """
#include "inspector.h"
#include "executor.h"
#include "utils.h"

/****************************/
// Inspector's ctypes-compatible data structures and functions
typedef struct {
  char* name;
  int core;
  int exec;
  int nonexec;
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

typedef struct {
  char* name;
  int* part;
  int size;
  int nparts;
} slope_part;

extern "C" void* inspector(slope_set sets[%(n_sets)d],
                           slope_map maps[%(n_maps)d],
                           int tileSize,
                           int rank,
                           slope_dat coords_dat[1],
                           slope_map* mesh_maps,
                           slope_part* partitionings);
/****************************/

void* inspector(slope_set sets[%(n_sets)d],
                slope_map maps[%(n_maps)d],
                int tileSize,
                int rank,
                slope_dat coords_dat[1],
                slope_map* mesh_maps,
                slope_part* partitionings)
{
  // Declare sets, maps, dats
  %(set_defs)s

  %(map_defs)s

  %(desc_defs)s

  // Additional maps defining the topology of the mesh (used for partitioning through METIS)
  map_list* meshMaps = new map_list();
  %(mesh_map_defs)s

  // Additional set partitionings (may be used to avoid explicit seed loop partitioning)
  map_list* setPartitionings = new map_list();
  %(partitionings_defs)s

  int avgTileSize = tileSize;
  int prefetchHalo = %(prefetchHalo)d;
  bool ignoreWAR = %(ignore_war)s;
  inspector_t* insp = insp_init (avgTileSize, %(mode)s, %(coloring)s, %(mesh_map_list)s,
                                 %(partitionings_list)s, prefetchHalo, ignoreWAR, %(name)s);

  %(loop_defs)s

  int seedTilePoint = %(seed)d;
  insp_run (insp, seedTilePoint);

  if (rank == 0) {
    %(output_insp)s
    %(output_vtk)s
  }

  executor_t* exec = exec_init (insp);
  insp_free (insp);
  delete meshMaps;
  return exec;
}
"""

    def __init__(self, name, **kwargs):
        self._name = name

        self._sets = kwargs.get('sets', [])
        self._maps = kwargs.get('maps', [])
        self._loops = kwargs.get('loops', [])

        self._mesh_maps = kwargs.get('mesh_maps', [])
        self._partitionings = kwargs.get('partitionings', [])

        self._part_mode = kwargs.get('part_mode', 'chunk')
        self._coloring = kwargs.get('coloring', 'default')
        self._prefetch = kwargs.get('prefetch', 0)
        self._seed_loop = kwargs.get('seed_loop', None)
        self._ignore_war = kwargs.get('ignore_war', False)

    def add_sets(self, sets):
        """Add ``sets`` to this Inspector

        :param sets: iterator of 5-tuple:
                     (name, core_size, exec_size, nonexec_size, superset)
        """
        # Subsets should come at last to avoid "undefined identifiers" when
        # compiling the generated code
        sets = sorted(list(sets), key=lambda x: x[4])
        # Now extract and format info for each set
        sets = [(_fix_c(n), cs, es, ns, _fix_c(sset)) for n, cs, es, ns, sset in sets]
        ctype = Set*len(sets)
        self._sets = sets
        return (ctype, ctype(*[Set(n, cs, es, ns) for n, cs, es, ns, sset in sets]))

    def add_maps(self, maps):
        """Add ``maps`` to this Inspector

        :param maps: iterator of 4-tuple:
                     (name, input_set, output_set, map_values)
        """
        maps = [(_fix_c(n), _fix_c(i), _fix_c(o), v) for n, i, o, v in maps]
        ctype = Map*len(maps)
        self._maps = maps
        return (ctype, ctype(*[Map(n, v.ctypes.data_as(ctypes.POINTER(ctypes.c_int)),
                                   v.size) for n, _, _, v in maps]))

    def add_loops(self, loops):
        """Add a list of ``loops`` to this Inspector

        :param loops: iterator of 3-tuple ``(name, set, desc)``, where:
            * name: the identifier of the loop
            * set: the iteration space of the loop
            * desc: represents a list of descriptors. In SLOPE, a descriptor
                    specifies the memory access pattern in a loop. In particular,
                    a descriptor is a 2-tuple in which the first entry is a map
                    (previously defined through a call to ``map(...)``) and the
                    second entry is the access mode (e.g., RW, READ, INC, ...).
                    If the access to a dataset does not involve any map, than the
                    first entry assumes the value of the special keyword ``DIRECT``
        """
        self._loops = [(_fix_c(n), _fix_c(s), [(_fix_c(i[0]), i[1]) for i in d])
                       for n, s, d in loops]

    def add_partitionings(self, partitionings):
        """Add sets partitionings. Can be used by SLOPE to derive tiles.

        :param partitionins: iterator of 2-tuple ``(set, nparray)``, where the nparray
                             represents a map from iterations to partition ids
        """
        partitionings = [("%s_to_partitioning" % _fix_c(s), _fix_c(s),
                          '%s_partitioning' % _fix_c(s), v) for s, v in partitionings]
        ctype = Part*len(partitionings)
        self._partitionings = partitionings
        return (ctype, ctype(*[Part(s, v.ctypes.data_as(ctypes.POINTER(ctypes.c_int)),
                                    v.size, max(v)) for n, _, _, v in partitionings]))

    def set_tile_size(self, tile_size):
        """Set a tile size for this Inspector"""
        ctype = ctypes.c_int
        return (ctype, ctype(tile_size))

    def set_mpi_rank(self, rank):
        """Inform about the process MPI rank."""
        ctype = ctypes.c_int
        return (ctype, ctype(rank))

    def drive_inspection(self, **kwargs):
        """Provide additional information to optimize the inspection phase.
        This method should be called prior to ``generate_code``.

        :param kwargs:
            * 'ignore_war': inform the inspector that for the given loop chain
                there is no need to track write-after-read dependencies.
            * 'seed_loop': set the loop from which tiles are derived.
            * 'prefetch': a value N such that N >= 0, default N = 0. N is the
                software prefetch distance that will be used by the executor.
            * 'coloring': set a coloring mode (available: ``default``, ``rand``,
                ``mincols``). May be used to improve load balancing.
            * 'part_mode': set the seed loop partitioning mode (available:
                ``chunk``, ``metis``). May be used to improve load balancing.
        """
        self._ignore_war = kwargs.get('ignore_war', False)

        self._seed_loop = kwargs.get('seed_loop', 0)
        assert self._seed_loop >= 0 and self._seed_loop < len(self._loops)

        self._prefetch = kwargs.get('prefetch', 0)
        assert isinstance(self._prefetch, int) and self._prefetch >= 0

        self._coloring = kwargs.get('coloring', 'default')
        assert self._coloring in ['default', 'rand', 'mincols']

        self._part_mode = kwargs.get('part_mode', 'chunk')
        assert self._part_mode in ['chunk', 'metis']

    def add_extra_info(self):
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
        if not (self._sets and self._loops):
            raise SlopeError("Loop chain not constructed yet")
        extra = []

        # Add coordinate field
        coordinates = Inspector._globaldata.get('coordinates')
        ctype = Dat*1
        if coordinates:
            set, data, arity = coordinates
            set_size = data.size / arity
            extra.append((ctype, ctype(Dat(data.ctypes.data_as(ctypes.POINTER(ctypes.c_double)),
                                           set_size))))
        else:
            extra.append((ctype, ctype(Dat(None, 0))))

        # Add mesh maps
        mesh_maps = Inspector._globaldata.get('mesh_maps', [])
        ctype = Map*len(mesh_maps)
        self._mesh_maps = mesh_maps
        extra.append((ctype, ctype(*[Map(n,
                                         v.ctypes.data_as(ctypes.POINTER(ctypes.c_int)),
                                         v.size) for n, _, _, v in mesh_maps])))

        return extra

    def generate_code(self):
        set_defs = [Inspector.set_def % (s[0], i, i, i, i, s[4] if s[4] else "NULL")
                    for i, s in enumerate(self._sets)]
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

        avail = lambda s: all(i in [s[0] for s in self._sets] for i in s)

        mesh_map_defs, mesh_map_list = "", "NULL"
        if self._mesh_maps and self._part_mode == 'metis':
            mesh_map_defs = [Inspector.mesh_map_def % ("mm_%s" % m[0], i, m[1], m[2], i, i)
                             for i, m in enumerate(self._mesh_maps) if avail([m[1], m[2]])]
            mesh_map_defs += ["meshMaps->insert(mm_%s);" % m[0]
                              for m in self._mesh_maps if avail([m[1], m[2]])]
            mesh_map_list = "meshMaps"

        partitionings_defs, partitionings_list = "", "NULL"
        if self._partitionings:
            partitionings_defs = [Inspector.part_def % (m[2], i, i, m[0], '"%s"' % m[0], m[1],
                                  m[2], i, i) for i, m in enumerate(self._partitionings)
                                  if avail([m[1]])]
            partitionings_defs += ["setPartitionings->insert(%s);" % m[0]
                                   for m in self._partitionings if avail([m[1]])]
            partitionings_list = "setPartitionings"

        coloring = "COL_%s" % self._coloring.upper()

        seed_loop = self._seed_loop if self._seed_loop is not None else len(self._loops) / 2

        debug_mode = Inspector._globaldata.get('debug_mode')
        coordinates = Inspector._globaldata.get('coordinates')
        output_insp, output_vtk = "", ""
        if debug_mode:
            output_insp = Inspector.output_insp % debug_mode
            if coordinates and coordinates[0] in [s[0] for s in self._sets]:
                output_vtk = Inspector.output_vtk % (debug_mode, coordinates[0],
                                                     "DIM%d" % coordinates[2])

        return Inspector.code % {
            'set_defs': "\n  ".join(set_defs),
            'map_defs': "\n  ".join(map_defs),
            'desc_defs': "\n  ".join(desc_defs),
            'loop_defs': "\n  ".join(loop_defs),
            'n_maps': len(self._maps),
            'n_sets': len(self._sets),
            'mode': Inspector._globaldata['mode'],
            'coloring': coloring,
            'prefetchHalo': self._prefetch,
            'seed': seed_loop,
            'mesh_map_defs': "\n  ".join(mesh_map_defs),
            'mesh_map_list': mesh_map_list,
            'partitionings_defs': "\n  ".join(partitionings_defs),
            'partitionings_list': partitionings_list,
            'ignore_war': str(self._ignore_war).lower(),
            'name': '"%s"' % self._name,
            'output_vtk': output_vtk,
            'output_insp': output_insp
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
        'headers': ['#include "%s"' % h for h in ['inspector.h', 'executor.h', 'utils.h']],
        'ctype_exec': 'void*',
        'py_ctype_exec': ctypes.POINTER(ctypes.c_void_p),
        'ctype_region_flag': 'tile_region',
        'region_flag': 'region',
        'ctype_rank': 'int',
        'rank': 'rank'
    }

    omp_code = """
#pragma omp parallel for schedule(dynamic)
"""
    init_code = """
executor_t* %(name_exec)s = (executor_t*)_%(name_exec)s;
int nColors = exec_num_colors (%(name_exec)s);
"""
    outer_tiles_loop = """\
for (int i = 0; i < nColors; i++) {
  const int nTilesPerColor = exec_tiles_per_color (%(name_exec)s, i);
  %(omp)s
  for (int j = 0; j < nTilesPerColor; j++) {
    // execute tile j for color i
    tile_t* tile = exec_tile_at (%(name_exec)s, i, j, %(region_flag)s);
    if (! tile) {
      // this might be the case if the tile region does not match
      continue;
    }
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
tileLoopSize = tile_loop_size (tile, %(loop_id)d);
"""

    debug_init = """
// per loop info
int nIters[%(nloops)s] = {0};
std::vector<double> times[%(nloops)d];
double start, end;
// directory in which results will be stored
struct stat st = {0};
if (stat("Execution_output", &st) == -1) {
  mkdir("Execution_output", 0700);
}\n
"""
    debug_end = """\n
std::string regionName = "core";
if (region == 1) {
  regionName = "exec";
}
std::stringstream stream;
stream << "Execution_output" << "/" << "%(name)s" << "_rank" << rank << "_" << regionName << ".txt";
std::ofstream outfile;
outfile.open (stream.str(), std::fstream::in | std::fstream::out | std::fstream::app);

for (int i = 0; i < %(nloops)d; i++) {
  int nTiles = times[i].size();
  double tot = 0.0;
  for (int j = 0; j < nTiles; j++) {
    tot += times[i][j];
  }

  outfile << "Loop " << i << ": totIters=" << nIters[i] << ", time=" << tot << std::endl;
}
outfile << "****************************" << std::endl;
outfile.close();
"""
    time_start = """
start = time_stamp();
"""
    time_end = """
end = time_stamp();
times[%(loop_id)d].push_back(end - start);
nIters[%(loop_id)d] += tileLoopSize;
"""

    def __init__(self, inspector):
        code_dict = dict(Executor.meta)
        code_dict.update({'omp': self._omp_pragma()})
        self._code = "\n".join([self._debug_init(inspector._loops),
                                Executor.init_code % code_dict,
                                Executor.outer_tiles_loop % code_dict,
                                self._debug_end(inspector._name, inspector._loops)])
        self._loop_init, self._gtl_maps, self._loop_end = self._genloops(inspector._loops)

    def _genloops(self, loops):
        """Return a 3-tuple, in which:

            * the first entry represents initialization code to be executed right
              before the kernel invocation
            * the second entry is a list of dict that binds, for each loop, global
              maps to local maps, as well as local iterations to the actual tile's
              iterations.
            * the last entry represents "clean up code" to be executed right after
              the kernel invocation
        """
        # Note: gtl stands for global-to-local
        header_code, gtl_maps, end_code = [], [], []
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
            if Inspector._globaldata.get('time_mode'):
                header_code[-1] += Executor.time_start
                end_code.append(Executor.time_end % {'loop_id': i})
            else:
                end_code.append("")

        return (header_code, gtl_maps, end_code)

    def _debug_init(self, loops):
        init = ""
        if Inspector._globaldata.get('time_mode'):
            init = Executor.debug_init % {'nloops': len(loops)}
        return init

    def _debug_end(self, name, loops):
        end = ""
        if Inspector._globaldata.get('time_mode'):
            end = Executor.debug_end % {
                'name': name,
                'nloops': len(loops)
            }
        return end

    def _omp_pragma(self):
        return Executor.omp_code if Inspector._globaldata['mode'] in ['OMP', 'OMP_MPI'] else ''

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
    def c_loop_end(self):
        return self._loop_end

    @property
    def c_headers(self):
        headers = Executor.meta['headers']
        if Executor._globaldata.get('time_mode'):
            headers += ['#include <vector>']
        return headers

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


# Utility functions

def _fix_c(var):
    """Make string ``var`` a valid C literal by removing invalid characters."""
    if not var:
        return var
    for ch in ['/', '#']:
        var = var.replace(ch, '')
    return var.split('.')[-1]


# Utility functions for the caller

def get_compile_opts(compiler='gnu'):
    """Return a list of options that are expected to be used when compiling the
    inspector/executor code. Supported compilers: [gnu (default), intel]."""
    functional_opts = ['-std=c++11']
    debug_opts = []
    if Inspector._globaldata.get('coordinates'):
        debug_opts = ['-DSLOPE_VTK']
    optimization_opts = ['-O3']
    optimization_opts.append('-fopenmp')
    if Inspector._globaldata['mode'] == 'OMP':
        if compiler == 'intel':
            optimization_opts.append('-par-affinity=scatter,verbose')
    if compiler == 'intel':
        optimization_opts.extend(['-xHost', '-ip'])
    return functional_opts + debug_opts + optimization_opts


def get_lib_name():
    """Return default name of the shared object resulting from compiling SLOPE"""
    return "slope"


def get_include_dir():
    """Return the include directory relative to the main folder"""
    return "sparsetiling/include"


def get_lib_dir():
    """Return the lib directory relative to the main folder"""
    return "lib"


# Functions for setting global information for inspection and execution

def set_debug_mode(mode, coordinates=None):
    """Output useful information once inspection is terminated.

    :param mode: the verbosity level of debug mode (MINIMAL, VERY_LOW, LOW, MEDIUM, HIGH)
    :param coordinates: (optional) Add a coordinates field that allows the inspector to
        generate VTK files useful for debugging and visualization purposes.
    :type coordinates: a 3-tuple, in which the first entry is the set name the
        coordinates belong to; the second entry is a numpy array of coordinates values;
        the third entry indicates the dimension of the dataset (accepted [1, 2, 3],
        for 1D, 2D, and 3D datasets, respectively)
    """
    modes = ['MINIMAL', 'VERY_LOW', 'LOW', 'MEDIUM', 'HIGH']
    if mode not in modes:
        print("Warning: debugging set to MINIMAL (%s not in: %s)" % (mode, str(modes)))
        mode = 'MINIMAL'
    Inspector._globaldata['debug_mode'] = mode

    if coordinates:
        _, _, arity = coordinates
        if arity not in [1, 2, 3]:
            raise SlopeError("Arity should be a number in [1, 2, 3]")
        Inspector._globaldata['coordinates'] = coordinates


def set_time_mode(mode):
    """In time mode (default off) each iteration of the loop chain is timed
    and printed to file (one file per MPI rank)."""
    assert mode in [True, False]
    Inspector._globaldata['time_mode'] = mode


def set_mesh_maps(maps):
    """Add the mesh topology through maps from generic mesh components (e.g. edges,
    cells) to nodes."""
    maps = [(_fix_c(n), _fix_c(i), _fix_c(o), v) for n, i, o, v in maps]
    Inspector._globaldata['mesh_maps'] = maps


def exec_modes():
    return ['SEQUENTIAL', 'OMP', 'ONLY_MPI', 'OMP_MPI']


def set_exec_mode(mode):
    """Set an execution mode (accepted: [SEQUENTIAL, OMP, ONLY_MPI, OMP_MPI])."""
    Inspector._globaldata['mode'] = mode if mode in exec_modes() else 'SEQUENTIAL'


def get_exec_mode():
    """Return the execution mode."""
    return Inspector._globaldata['mode']
