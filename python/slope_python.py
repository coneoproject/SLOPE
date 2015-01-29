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
// Inspector's ctypes-compatible data structures
%(set_decl)s
/****************************/

void inspector(slope_set* sets) {

  %(set_defs)s

  std::cout << "Hello, World!" << std::endl;
  return;
}
    """

    def __init__(self):
        self._sets = []
        self._dats = []
        self._maps = []
        self._loops = []

    def get_arg_types(self):
        return [ctypes.POINTER(Set)]

    def add_set(self, name, size):
        new_set = Set(name, size)
        self._sets.append(new_set)
        return ctypes.byref(new_set)

    def generate_code(self):
        # Sets code
        set_defs = [Inspector.set_def % (s.name, i, i) for i, s in enumerate(self._sets)]

        return Inspector.code % {
            'set_decl': Inspector.struct_set,
            'set_defs': "\n  ".join(set_defs)
        }


# Utility functions for the caller

def get_lib_name():
    return "libst.a"
