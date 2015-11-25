#
# The following environment variable(s) should be predefined:
#
# CXX
# CXX_OPTS
# SLOPE_ARCH (mac,linux)
# SLOPE_LIB (static,shared)

#
# The following environment variable(s) can be predefined
#
# DEBUG: run in debug mode, printing additional information

#
# Set paths for various files
#

ST = sparsetiling
LIB = lib
OBJ = obj
DEMOS = demos
BIN = bin
TESTS = tests

ST_SRC = $(ST)/src
ST_INC = $(ST)/include
ST_BIN = $(BIN)/sparsetiling
ST_DEMOS = $(DEMOS)/sparsetiling
ST_TESTS = $(TESTS)/sparsetiling

ALL_OBJS = $(OBJ)/inspector.o $(OBJ)/partitioner.o $(OBJ)/coloring.o $(OBJ)/tile.o \
		   $(OBJ)/parloop.o $(OBJ)/tiling.o $(OBJ)/map.o $(OBJ)/executor.o $(OBJ)/utils.o

METIS_INC = $(SLOPE_METIS)/include
METIS_LIB = $(SLOPE_METIS)/lib
METIS_LINK = -L$(METIS_LIB) -lmetis

#
# Compiler settings
#

CXX := $(CXX)
MPICXX := $(MPICXX)
CXXFLAGS := -std=c++0x $(CXX_OPTS) $(SLOPE_VTK)

ifeq ($(SLOPE_ARCH),linux)
  CLOCK_LIB = -lrt
endif

ifeq ($(SLOPE_LIB),shared)
  LIBFLAGS = -fPIC
endif

.PHONY: clean mklib

all: warning clean mklib sparsetiling tests demos

warning:
	@echo "Recommended compiler options for maximum performance on an Intel processor:"
	@echo "-O3 -fopenmp -xAVX -inline-forceinline -ipo"

mklib:
	@mkdir -p $(LIB) $(OBJ) $(ST_BIN)/airfoil $(ST_BIN)/tests

sparsetiling: mklib
	@echo "Compiling the library"
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/inspector.cpp -o $(OBJ)/inspector.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/executor.cpp -o $(OBJ)/executor.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -I$(METIS_INC) -c $(ST_SRC)/partitioner.cpp -o $(OBJ)/partitioner.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/coloring.cpp -o $(OBJ)/coloring.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/map.cpp -o $(OBJ)/map.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tile.cpp -o $(OBJ)/tile.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/parloop.cpp -o $(OBJ)/parloop.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tiling.cpp -o $(OBJ)/tiling.o
	$(CXX) $(LIBFLAGS) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/utils.cpp -o $(OBJ)/utils.o
	ar cru $(LIB)/libst.a $(ALL_OBJS)
	ranlib $(LIB)/libst.a
ifeq ($(SLOPE_LIB),shared)
	$(CXX) -shared -Wl,-soname,libst.so.1 -o $(LIB)/libst.so.1.0.1 $(ALL_OBJS)
endif

tests: mklib
	@echo "Compiling the tests"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_TESTS)/test_loopchain_1.cpp -o $(ST_BIN)/tests/test_loopchain_1 $(LIB)/libst.a $(METIS_LINK) $(CLOCK_LIB)
	$(MPICXX) $(CXXFLAGS) -I$(ST_INC) $(ST_TESTS)/test_mpi.cpp -o $(ST_BIN)/tests/test_mpi $(LIB)/libst.a $(METIS_LINK) $(CLOCK_LIB)

demos: mklib
	@echo "Compiling the demos"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_DEMOS)/airfoil/airfoil.cpp -o $(ST_BIN)/airfoil/airfoil $(METIS_LINK) $(CLOCK_LIB)
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_DEMOS)/airfoil/airfoil_tiled.cpp -o $(ST_BIN)/airfoil/airfoil_tiled $(LIB)/libst.a $(METIS_LINK) $(CLOCK_LIB)

clean:
	@echo "Removing objects, libraries, executables"
	-rm -if $(OBJ)/*.o
	-rm -if $(OBJ)/*~
	-rm -if $(LIB)/*.a
	-rm -if $(LIB)/*.so
	-rm -if $(ST_SRC)/*~
	-rm -if $(ST_INC)/*~
	-rm -if $(ST_DEMOS)/airfoil/*~
	-rm -if $(ST_BIN)/airfoil/airfoil_*
	-rm -if $(ST_BIN)/tests/test_*
