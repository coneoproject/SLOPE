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
    pass


class Map(ctypes.Structure):
    pass


class Loop(ctypes.Structure):
    pass


class Inspector(object):

    ### Templates for code generation ###

    struct_set = """
typedef struct {
  char* name;
  int size;
} slope_set;
"""

    set_def = "set_t* set_%s = set(sets[%d].name, sets[%d].size);"

    code = """
#include <iostream>

#include "inspector.h"

/****************************/
// Inspector's ctypes-compatible data structures and functions
%(set_decl)s
extern "C" void inspector(slope_set sets[%(n_sets)d]);
/****************************/

void inspector(slope_set sets[%(n_sets)d]) {

  %(set_defs)s

  std::cout << "Hello, World!" << std::endl;
  return;
}
"""

    def __init__(self):
        self._sets, self._dats, self._maps, self._loops = [None]*4

    def get_arg_types(self):
        """This method must be called only after the Inspector has fully been
        defined. Here ``fully defined`` means that calls to ``add_sets``,
        ``add_dats``, ``add_maps``, and ``add_loops`` must have performed,
        which is a requirement to return the correct data types. In such an
        event, a ``SlopeError`` exception is raised."""
        try:
            return [self._sets._type_*len(self._sets)]
        except TypeError:
            raise SlopeError("Cannot return arguments types prior to initialization")

    def add_sets(self, sets):
        """Add ``sets`` to this Inspector

        :param sets: iterator of 2-tuple, in which the first entry is the name of
                     the set (a string), while the second entry is the size of the
                     iteration set
        """
        sets = (Set*len(sets))(*[Set(name, size) for name, size in sets])
        self._sets = sets
        return sets

    def generate_code(self):
        # Sets code
        set_defs = [Inspector.set_def % (s.name, i, i) for i, s in enumerate(self._sets)]

        return Inspector.code % {
            'set_decl': Inspector.struct_set,
            'set_defs': "\n  ".join(set_defs),
            'n_sets': len(self._sets)
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
