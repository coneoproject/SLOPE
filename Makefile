#
# The following environment variable(s) should have been set:
#
# SLOPE_ARCH (mac,linux) [default: linux]
# SLOPE_COMPILER (gnu,intel) [default: gnu]
# CXX_OPTS [default: -O3]
# MPICXX [default: mpicc]

#
# The following environment variable(s) can be predefined
#
# DEBUG: run in debug mode, printing additional information

#
# Set paths for various files
#

# OP2=1
# DEBUG=1
# SLOPE_COMPILER = intel
SLOPE_OMP=-fopenmp
# SLOPE_METIS=/opt/metis/5.1.0	# commented to make the default partitioner to chunk
#SLOPE_VTK = -DSLOPE_VTK
CXX_OPTS = 
FT_OPTS = 

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
		   $(OBJ)/parloop.o $(OBJ)/tiling.o $(OBJ)/map.o $(OBJ)/executor.o $(OBJ)/utils.o \
		   $(OBJ)/schedule.o $(OBJ)/set.o $(OBJ)/descriptor.o

#
# Compiler settings
#

OS := $(shell uname)
CXX := g++
MPICXX := mpicc
CXXFLAGS := $(CXXFLAGS) -std=c++0x -fPIC $(CXX_OPTS) $(SLOPE_VTK) 
CLOCK_LIB = -lrt

ifeq ($(SLOPE_COMPILER),gnu)
  CXX := g++
endif
ifeq ($(SLOPE_COMPILER),intel)
  CXX := icpc
endif

ifdef MPICXX
  MPICXX := $(MPICXX)
endif

ifdef SLOPE_OMP
  CXXFLAGS := $(CXXFLAGS) -DSLOPE_OMP $(SLOPE_OMP)
  FFLAGS := $(FFLAGS) -DSLOPE_OMP $(SLOPE_OMP)
endif

ifdef SLOPE_METIS
  METIS_INC = -I$(SLOPE_METIS)/include
  METIS_LIB = $(SLOPE_METIS)/lib
  METIS_LINK = -L$(METIS_LIB) -lmetis

  CXXFLAGS := $(CXXFLAGS) -DSLOPE_METIS
  FFLAGS := $(FFLAGS) -DSLOPE_METIS
endif

ifdef DEBUG
	ifeq ($(SLOPE_COMPILER),gnu)
		CXXFLAGS := $(CXXFLAGS) -O3 -g
		FFLAGS := $(FFLAGS) -O3 -g
	else
		CXXFLAGS := $(CXXFLAGS) -O0 -g -traceback
		FFLAGS := $(FFLAGS) -O0 -g -traceback
	endif
else
  CXXFLAGS := $(CXXFLAGS) -O3
  FFLAGS := $(FFLAGS) -O3
endif

# ifdef OP2
#   CXXFLAGS := $(CXXFLAGS) -DOP2
#   FFLAGS := $(FFLAGS) -DOP2
# endif

ifeq ($(OS),Linux)
  SONAME := -soname
endif
ifeq ($(OS),Darwin)
  SONAME := -install_name
endif

.PHONY: clean mklib

all: sparsetiling op2_sparsetiling

full: warning clean mklib sparsetiling tests demos

warning:
	@echo "Recommended compiler options for maximum performance on an Intel processor:"
	@echo "-O3 -fopenmp -xAVX -inline-forceinline -ipo"

mklib:
	@mkdir -p $(LIB) $(OBJ) $(ST_BIN)/airfoil $(ST_BIN)/tests

sparsetiling: mklib
	@echo "Compiling the library"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/inspector.cpp -o $(OBJ)/inspector.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/executor.cpp -o $(OBJ)/executor.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(METIS_INC) -c $(ST_SRC)/partitioner.cpp -o $(OBJ)/partitioner.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/coloring.cpp -o $(OBJ)/coloring.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/map.cpp -o $(OBJ)/map.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tile.cpp -o $(OBJ)/tile.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/parloop.cpp -o $(OBJ)/parloop.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tiling.cpp -o $(OBJ)/tiling.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/schedule.cpp -o $(OBJ)/schedule.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/utils.cpp -o $(OBJ)/utils.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/set.cpp -o $(OBJ)/set.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/descriptor.cpp -o $(OBJ)/descriptor.o
	ar cru $(LIB)/libslope.a $(ALL_OBJS)
	ranlib $(LIB)/libslope.a
	$(CXX) -shared -Wl,$(SONAME),libslope.so -o $(LIB)/libslope.so $(ALL_OBJS) $(METIS_LINK)

op2_sparsetiling: mklib
	@echo "Compiling the library"
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/inspector.cpp -o $(OBJ)/inspector.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/executor.cpp -o $(OBJ)/executor.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) $(METIS_INC) -c $(ST_SRC)/partitioner.cpp -o $(OBJ)/partitioner.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/coloring.cpp -o $(OBJ)/coloring.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/map.cpp -o $(OBJ)/map.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/tile.cpp -o $(OBJ)/tile.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/parloop.cpp -o $(OBJ)/parloop.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/tiling.cpp -o $(OBJ)/tiling.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/schedule.cpp -o $(OBJ)/schedule.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/utils.cpp -o $(OBJ)/utils.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/set.cpp -o $(OBJ)/set.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/descriptor.cpp -o $(OBJ)/descriptor.o
	ar cru $(LIB)/libop2slope.a $(ALL_OBJS)
	ranlib $(LIB)/libop2slope.a
	$(CXX) -shared -Wl,$(SONAME),libop2slope.so -o $(LIB)/libop2slope.so $(ALL_OBJS) $(METIS_LINK)

tests: mklib
	@echo "Compiling the tests"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_TESTS)/test_loopchain_1.cpp -o $(ST_BIN)/tests/test_loopchain_1 $(LIB)/libslope.a $(METIS_LINK) $(CLOCK_LIB)
	$(MPICXX) $(CXXFLAGS) -I$(ST_INC) $(ST_TESTS)/test_mpi.cpp -o $(ST_BIN)/tests/test_mpi $(LIB)/libslope.a $(METIS_LINK) $(CLOCK_LIB)

demos: mklib
	@echo "Compiling the demos"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_DEMOS)/airfoil/airfoil.cpp -o $(ST_BIN)/airfoil/airfoil $(METIS_LINK) $(CLOCK_LIB)
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(ST_DEMOS)/airfoil/airfoil_tiled.cpp -o $(ST_BIN)/airfoil/airfoil_tiled $(LIB)/libslope.a $(METIS_LINK) $(CLOCK_LIB)

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
	-rm -if $(FT_MOD)/*.mod
	-rm -if $(FT_MOD)/*~
	-rm -if $(FT_LIB)/*.a
	-rm -if $(FT_LIB)/*.so


FT = fortran
FT_SRC = $(FT)/sparsetiling/src
FT_INC = $(FT)/sparsetiling/include
FT_BIN = $(FT)/bin/sparsetiling
FT_LIB = $(FT)/lib
FT_OBJ = $(FT)/obj
FT_MOD = $(FT)/mod

FT_ALL_OBJS = $(FT_OBJ)/sl_for_declarations.o

ifeq ($(SLOPE_COMPILER),gnu)
  FT_INC_MOD = $(FT_MOD)
  FC = ftn
  FFLAGS := $(FFLAGS) -J$(FT_INC_MOD) -fPIC $(FT_OPTS) $(SLOPE_VTK)
endif
ifeq ($(SLOPE_COMPILER),intel)
	FT_INC_MOD = $(FT_MOD)
	FC = ifort
	FFLAGS := $(FFLAGS) -module $(FT_INC_MOD) -fPIC $(FT_OPTS) $(SLOPE_VTK)
endif

ft_all: ft_sparsetiling op2_ft_sparsetiling

ft_mklib:
	@mkdir -p $(FT_LIB) $(FT_OBJ) $(FT_MOD) $(FT_BIN)/airfoil $(FT_BIN)/tests

ft_sparsetiling: ft_mklib
	@echo "Compiling the Fortran library"
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/inspector.cpp -o $(OBJ)/inspector.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/executor.cpp -o $(OBJ)/executor.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) $(METIS_INC) -c $(ST_SRC)/partitioner.cpp -o $(OBJ)/partitioner.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/coloring.cpp -o $(OBJ)/coloring.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/map.cpp -o $(OBJ)/map.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tile.cpp -o $(OBJ)/tile.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/parloop.cpp -o $(OBJ)/parloop.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/tiling.cpp -o $(OBJ)/tiling.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/schedule.cpp -o $(OBJ)/schedule.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/utils.cpp -o $(OBJ)/utils.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/set.cpp -o $(OBJ)/set.o
	$(CXX) $(CXXFLAGS) -I$(ST_INC) -c $(ST_SRC)/descriptor.cpp -o $(OBJ)/descriptor.o
	$(FC) $(FFLAGS) -c $(FT_SRC)/sl_for_declarations.F90 -o $(FT_OBJ)/sl_for_declarations.o
	ar -r $(FT_LIB)/libfslope.a $(ALL_OBJS) $(FT_ALL_OBJS)
	ranlib $(FT_LIB)/libfslope.a
	$(CXX) -shared -Wl,$(SONAME),libfslope.so -o $(FT_LIB)/libfslope.so $(ALL_OBJS) $(FT_ALL_OBJS) $(METIS_LINK)

op2_ft_sparsetiling: ft_mklib
	@echo "Compiling the Fortran library"
	$(CXX) $(CXXFLAGS) -DOP2  -I$(ST_INC) -c $(ST_SRC)/inspector.cpp -o $(OBJ)/inspector.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/executor.cpp -o $(OBJ)/executor.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) $(METIS_INC) -c $(ST_SRC)/partitioner.cpp -o $(OBJ)/partitioner.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/coloring.cpp -o $(OBJ)/coloring.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/map.cpp -o $(OBJ)/map.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/tile.cpp -o $(OBJ)/tile.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/parloop.cpp -o $(OBJ)/parloop.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/tiling.cpp -o $(OBJ)/tiling.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/schedule.cpp -o $(OBJ)/schedule.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/utils.cpp -o $(OBJ)/utils.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/set.cpp -o $(OBJ)/set.o
	$(CXX) $(CXXFLAGS) -DOP2 -I$(ST_INC) -c $(ST_SRC)/descriptor.cpp -o $(OBJ)/descriptor.o
	$(FC) $(FFLAGS) -c $(FT_SRC)/sl_for_declarations.F90 -o $(FT_OBJ)/sl_for_declarations.o
	ar -r $(FT_LIB)/libfop2slope.a $(ALL_OBJS) $(FT_ALL_OBJS)
	ranlib $(FT_LIB)/libfop2slope.a
	$(CXX) -shared -Wl,$(SONAME),libfop2slope.so -o $(FT_LIB)/libfop2slope.so $(ALL_OBJS) $(FT_ALL_OBJS) $(METIS_LINK)

	
