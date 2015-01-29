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

    code = """
    #include <stdio.h>

    %(set_decl)s

    void inspector(slope_set* tmp) {
      printf("Hello, World!\\n");
      printf("Set name: %%s, size: %%d\\n", tmp[0].name, tmp[0].size);
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
        return Inspector.code % {
            'set_decl': Inspector.struct_set
        }
